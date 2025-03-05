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

/** @file libera_opcua.h
  OpcUaServer : MCI access
  Version 0.1 2025/01/30
  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
  
  The OPC-UA library and the MCI access need different incompatible
  compiler parameters. Therefore, the instrument access is divided
  into 2 layers, one providing the MCI access and one
  handling the OPC-UA variables on top of it.
 */

#include <stdint.h>

#ifndef LIBERAOPCUA_H
#define LIBERAOPCUA_H

#include "libera_mci.h"      // the MCI access layer
#include "open62541.h"       // the OPC UA library

#ifdef __cplusplus
extern "C" {
#endif

//*************************************
// read/write methods for MCI variables
// code by code_generator.py
//*************************************

#include "libera_opcua.h.inc"

/***********************************/
/* OPC-UA data source              */
/* read/write methods              */
/***********************************/

#ifdef __cplusplus
} // extern "C"
#endif

#endif
