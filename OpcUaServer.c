/*
MIT License

Copyright (c) 2025 Ulf Lehnert, Helmholtz-Center Dresden-Rossendorf

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/** @file OpcUaServer.c
 *
 *  @mainpage OpcUaServer for Libera Digit 500
 *
 *  Version 0.1  30.1.2025
 *
 *  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
 *
 *  It is based on the [Open62541](https://github.com/open62541/open62541/)
 *  open source implementation of OPC UA.
 *
 *  The OPC UA stack needs to be downloaded and built. This can be done on
 *  the development system - there is no binary code produced at this stage.
 *  The complete stack is obtained in two (amalgamated) files.
 *  - open62541.h
 *  - open62541.c
 *
 *  @section Build
 *
 *  Presently, no makefiles are provided. The server can be built
 *  with only a few commands utilizing a cross-compiler tool chain.
 *
 *  source ~/Libera/tools/environment
 *  $CC -c -std=c99 open62541.c
 *  $CXX -c -std=gnu++11 -I. -L$SDKTARGETSYSROOT/opt/libera/lib libera_mci.cpp
 *  $CC -c -std=c99 -I. libera_opcua.c
 *  $CC -c -std=c99 -I. OpcUaServer.c
 *  $CXX -o opcua_server OpcUaServer.o open62541.o libera_mci.o libera_opcua.o  -lpthread -L$SDKTARGETSYSROOT/opt/libera/lib -lliberamci -lliberaisig -lliberaistd -lliberainet -lomniORB4 -lomniDynamic4 -lomnithread
 *
 *
 *  @section Testing
 *  For a first test of the server an universal OPC UA client like
 *  [UaExpert](https://www.unified-automation.com/products/development-tools/uaexpert.html) is recommended.
 */

#include <unistd.h>
#include <stdio.h>
#include <signal.h>		     // for signal()
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>        // for fstat()
#include <pthread.h>         // for threads

#include "open62541.h"       // the OPC-UA library
#include "libera_opcua.h"

/***********************************/
/* Server-related variables        */
/***********************************/

// the OPC-UA server
static UA_Server *server;

// this variable is a flag for the running server
// when set to false the server stops
static volatile UA_Boolean running = true;

/***********************************/
/* interrupt and error handling    */
/***********************************/

// print error message and abort the running program
static void Die(const char *mess)
{
    UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", mess);
    exit(1);
}

// handle SIGINT und SIGTERM
static void stopHandler(int signal)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/***********************************/
/* data stream extracted variables */
/***********************************/

// the data structure sent by the Libera instrument
typedef struct {
   int32_t Ch1_rss;
   int32_t Ch1_peak;
   int32_t Ch1_avg;
   int32_t Ch1_sum;
   int32_t Ch2_rss;
   int32_t Ch2_peak;
   int32_t Ch2_avg;
   int32_t Ch2_sum;
   int32_t Ch3_rss;
   int32_t Ch3_peak;
   int32_t Ch3_avg;
   int32_t Ch3_sum;
   int32_t Ch4_rss;
   int32_t Ch4_peak;
   int32_t Ch4_avg;
   int32_t Ch4_sum;
} pulse_data;
#define BLOCKSIZE 64

// This is the last received data block from the stream.
// It is written asynchronously by the receiver thread.
// While the thread is writing the block the according semaphore is set.
static volatile pulse_data stream_data_block;
static volatile bool stream_data_block_writing_active = false;
static volatile int32_t pulse_counter = 0;
static volatile int32_t pulse_stream_pps = 0;

// Read the data from the pulse-processing stream and write into the global data block.
// This procedure will be forked off as a parallel thread.
// It needs the file descriptor of the open stream as a parameter.
// It runs until the OPC UA server is stopped.
void* read_pulse_Stream(void *arg)
{
    char readbuffer[BLOCKSIZE];             // buffer for reading from the data stream
    
    int fd = *((int *)arg);
    printf("OpcUaServer : reading from fd=%d\n",fd);

    while (running)
    {
        int bytes_read = read(fd, readbuffer, BLOCKSIZE);
        // handle read errors
        if (-1 == bytes_read)
        {
            int errsv=errno;
            fprintf(stderr, "OpcUaServer : read() from data stream");
            sleep(0.1);
        };
        // handle proper data blocks
        if (BLOCKSIZE==bytes_read)
        {
            stream_data_block_writing_active = true;
            pulse_counter++;
            // copy data from buffer to struct
            memcpy((void *) &stream_data_block, readbuffer, BLOCKSIZE);
            stream_data_block_writing_active = false;
        };
    };
    printf("OpcUaServer : read thread exit\n");
    pthread_exit(NULL);
}

// Once every second the pulse repetition rate is computed
// and the pulse counter reset to zero.
void* timer_thread(void* arg) {
    (void)arg;  // Unused parameter
    while (running) {
        sleep(1.0);
        stream_data_block_writing_active = true;
        pulse_stream_pps = pulse_counter;
        pulse_counter = 0;
        stream_data_block_writing_active = false;
    }
    printf("OpcUaServer : timer thread exit\n");
    pthread_exit(NULL);
}


/***********************************/
/* generic read/write methods      */
/* for server-internal variables   */
/***********************************/

static UA_StatusCode read_UA_Int32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    // wait for semaphore because
    // this read method is mainly used for data stream related variables
    while (stream_data_block_writing_active);
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Int32*)nodeContext, &UA_TYPES[UA_TYPES_INT32]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode write_UA_Int32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if (UA_Variant_isScalar(&(data->value)) && data->value.type == &UA_TYPES[UA_TYPES_INT32] && data->value.data)
    {
        *(UA_Int32*)nodeContext = *(UA_Int32*)data->value.data;
    }
    return UA_STATUSCODE_GOOD;
}

/***********************************/
/* main program                    */
/***********************************/

int main(int argc, char** argv)
{

    mci_init();
    
    // server will be running until we receive a SIGINT or SIGTERM
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    // server = UA_Server_new();
    
    // configure the UA server - port 10001
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, 10001, NULL);
    if(res != UA_STATUSCODE_GOOD)
    {
        // printf("UA_ServerConfig_setMinimal() error %8x\n", res);
        exit(-1);
    }
    server = UA_Server_newWithConfig(&config);
    if(!server)
    {
        // printf("UA_Server_newWithConfig() failed\n");
        exit(-1);
    }

    //**************************************
    // capture the pulse data stream
    //**************************************

    // open the data stream
    int stream_fd = open("/dev/libera.strm0", O_RDONLY);
    if (stream_fd == -1)
    {
        Die("OpcUaServer : failed to open /dev/libera.strm0");
    } else {
        printf("opened /dev/libera.strm0 with fd=%d\n",stream_fd);
    };

    // fork off a thread that reads the stream data
    pthread_t stream_tid;
    if (0 != pthread_create(&stream_tid, NULL, &read_pulse_Stream, (void *)&stream_fd))
        Die("OpcUaServer : failed to create pulse read thread");
    else
        printf("OpcUaServer : pulse read thread created successfully\n");

    // fork off a thread for the pulse counting
    pthread_t timer_tid;
    if (0 != pthread_create(&timer_tid, NULL, timer_thread, NULL))
        Die("OpcUaServer : failed to create timer thread");
    else
        printf("OpcUaServer : timer thread created successfully\n");

    //**************************************
    // create and populate the device folder
    // code by code_generator.py
    //**************************************

    UA_ObjectAttributes object_attr;   // attributes for folders
    UA_VariableAttributes attr;        // attributes for variable nodes

    #include "OpcUaServer.c.inc"

    //**************************************
    // add manually coded variables
    //**************************************
    
    // run the server (forever unless stopped with ctrl-C)
    UA_StatusCode retval = UA_Server_run(server, &running);
    
    // the server has stopped running
    if(retval != UA_STATUSCODE_GOOD)
        printf("UA_Server_run() error %8x\n", retval);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "server stopped running.");
    UA_Server_delete(server);
    // nl.deleteMembers(&nl);

    // wait for the read and timer threads to exit
    pthread_join(stream_tid, NULL);
    pthread_join(timer_tid, NULL);

    int status = close(stream_fd);
    if (-1==status)
        perror("OpcUaServer : problems closing source stream");
    else
        printf("OpcUaServer : data stream closed.\n");

    mci_shutdown();
    
    printf("OpcUaServer : graceful exit\n");
    
    return (int)retval;
}

