/*
MIT License

Copyright (c) 2017 Ulf Lehnert, Helmholtz-Center Dresden-Rossendorf

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

/** @file libera_mci.c
  OpcUaServer : MCI access
  Version 0.1 2025/01/30
  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
 */

#include <stdio.h>
#include <stdlib.h>

#include "mci/mci.h"

#include "libera_mci.h"

// global variables for persistent access
mci::Node node_root;
mci::Node node_pulse_processing_enable;
mci::Node node_pulse_processing_threshold;

//*************************************
// persistent variables for MCI nodes
// code by code_generator.py
//*************************************

#include "libera_mci.cpp.nodes.inc"

int mci_error;

int mci_init()
{
    mci::Init();
    // connect to LiberaBase application running on local host
    node_root = mci::Connect();
    if (node_root.IsValid())
    {
        printf("MCI connect OK\n");
        mci_error = 0;
    } else {
        printf("MCI error : can't connect\n");
        mci_error = 1;
    }

    // include code by code_generator.py
    #include "libera_mci.cpp.init.inc"    

    return(mci_error);
}

int mci_shutdown()
{
    mci::Shutdown();
    printf("MCI shutdown OK\n");
    return(0);
}

//*************************************
// read/write methods for MCI variables
// code by code_generator.py
//*************************************

#include "libera_mci.cpp.inc"

//*************************************
// read/write methods for MCI variables
// manually coded for special handling
//*************************************

// the variable is not boolean but enum type
// it can be read successfully either as an int or as a string
bool mci_get_pulse_enable(bool *val)
{
    // false=0 true=1
    int mci_enum;
    bool success = node_pulse_enable.GetValue(mci_enum);
    *val = (mci_enum==1);
    return success;
}
bool mci_set_pulse_enable(bool val)
{
    bool success;
    if (val)
        success = node_pulse_enable.SetValue("true");
    else
        success = node_pulse_enable.SetValue("false");
    return success;
}

