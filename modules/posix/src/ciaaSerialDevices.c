/* Copyright 2014, Mariano Cerdeiro
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief CIAA Serial Devices
 **
 ** Provides support for serial devices
 **
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup POSIX POSIX Implementation
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 * MaCe         Mariano Cerdeiro
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * 20140525 v0.0.1 initials initial version
 */

/*==================[inclusions]=============================================*/
#include "ciaaSerialDevices.h"
#include "ciaaPOSIX_stdio.h"
#include "ciaaPOSIX_stdint.h"
#include "ciaaPOSIX_string.h"
#include "ciaaPOSIX_assert.h"
#include "ciaaLibs_CircBuf.h"
#include "ciaak.h"
#include "os.h"

/*==================[macros and definitions]=================================*/
#define ciaaSerialDevices_MAXDEVICES   20

/*==================[typedef]================================================*/
typedef struct {
   ciaaDevices_deviceType const * device;
   TaskType taskID;
   ciaaLibs_CircBufType rxBuf;
   ciaaLibs_CircBufType txBuf;
   uint8_t flags;
} ciaaSerialDevices_deviceType;

/** \brief Serial Devices Type */
typedef struct {
   ciaaSerialDevices_deviceType devstr[ciaaSerialDevices_MAXDEVICES];
   uint8_t position;
} ciaaSerialDevices_devicesType;

/*==================[internal data declaration]==============================*/
/** \brief List of devices */
ciaaSerialDevices_devicesType ciaaSerialDevices;

/** \brief Prefix of all serial devices
 *
 * This prefix is used for all drevices which drivers are register against this
 * device.
 *
 * Eg. if a driver is uart/1 and it registers to this device type (/dev/serial)
 * the resulting path will be /dev/serial/uart/1
 */
char const * const ciaaSerialDevices_prefix = "/dev/serial";

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/
extern void ciaaSerialDevices_init(void)
{
   /* reset position of the drivers */
   ciaaSerialDevices.position = 0;
}

extern void ciaaSerialDevices_addDriver(ciaaDevices_deviceType * driver)
{
   ciaaDevices_deviceType * newDevice;
   char * newDeviceName;
   uint8_t length;
   uint8_t position;

   /* enter critical section */
   /* not needed, only 1 task running */

   /* check if more drivers can be added */
   if (ciaaSerialDevices_MAXDEVICES > ciaaSerialDevices.position) {

      /* get position for nexxt device */
      position = ciaaSerialDevices.position;

      /* increment position for next device */
      ciaaSerialDevices.position++;

      /* exit critical section */
      /* not needed, only 1 task running */

      /* add driver */
      ciaaSerialDevices.devstr[position].device = driver;

      /* configure rx and tx buffers */
      /* TODO buffer size shall be created depending on the device type (eth != uart) */
      ciaaLibs_circBufInit(&ciaaSerialDevices.devstr[position].rxBuf, ciaak_malloc(256), 256);
      ciaaLibs_circBufInit(&ciaaSerialDevices.devstr[position].txBuf, ciaak_malloc(256), 256);

      /* allocate memory for new device */
      newDevice = (ciaaDevices_deviceType*) ciaak_malloc(sizeof(ciaaDevices_deviceType));

      /* set functions for this device */
      newDevice->open = ciaaSerialDevices_open;
      newDevice->close = ciaaSerialDevices_close;
      newDevice->ioctl = ciaaSerialDevices_ioctl;
      newDevice->read = ciaaSerialDevices_read;
      newDevice->write = ciaaSerialDevices_write;

      /* store layers information information */
      newDevice->layer = (void *) &ciaaSerialDevices.devstr[position];
      newDevice->loLayer = (void *) driver;

      /* store newDevice layer information in the lower layer */
      driver->upLayer = newDevice;

      /* create path string for this device */
      length = ciaaPOSIX_strlen(driver->path);
      length += ciaaPOSIX_strlen(ciaaSerialDevices_prefix);
      length += 2; /* for the / and the termination null */

      /* create path for the new device */
      newDeviceName = (char *) ciaak_malloc(length);

      /* start a new string */
      *newDeviceName = 0;

      /* add prefix, / and the device name */
      ciaaPOSIX_strcat(newDeviceName, ciaaSerialDevices_prefix);
      ciaaPOSIX_strcat(newDeviceName, "/");
      ciaaPOSIX_strcat(newDeviceName, driver->path);
      /* add path to device structure */
      newDevice->path = newDeviceName;

      /* add device */
      ciaaDevices_addDevice(newDevice);
   }
   else
   {
      /* exit critical section */
      /* not needed, only 1 task running */
   }
}

extern ciaaDevices_deviceType * ciaaSerialDevices_open(char const * path,
      ciaaDevices_deviceType * device, uint8_t const oflag)
{
   /* serial devices does not support that the drivers update the device */
   /* the returned device shall be the same as passed */
   ciaaPOSIX_assert(device->open(path, (ciaaDevices_deviceType *)device->loLayer, oflag) == device->loLayer);

   return device;
}

extern int32_t ciaaSerialDevices_close(ciaaDevices_deviceType const * const device)
{
   return device->close((ciaaDevices_deviceType *)device->loLayer);
}

extern int32_t ciaaSerialDevices_ioctl(ciaaDevices_deviceType const * const device, int32_t request, void* param)
{
   int32_t ret;

   if (ciaaPOSIX_IOCTL_RXINDICATION == request)
   {

   }
   else
   {
      ret = device->ioctl((ciaaDevices_deviceType *)device->loLayer, request, param);
   }

   return ret;
}

extern int32_t ciaaSerialDevices_read(ciaaDevices_deviceType const * const device, uint8_t * const buf, uint32_t nbyte)
{
   /* get serial device */
   ciaaSerialDevices_deviceType * serialDevice =
      (ciaaSerialDevices_deviceType*) device->layer;
   int32_t ret = 0;

   /* if the rx buffer is not empty */
   if (!ciaaLibs_circBufEmpty(&serialDevice->rxBuf))
   {
      /* try to read nbyte from rxBuf and store it to the user buffer */
      ret = ciaaLibs_circBufGet(&serialDevice->rxBuf,
            buf,
            nbyte);
   }
   else
   {
      /* get task id for waking up the task later */
      GetTaskID(&serialDevice->taskID);

      /* if no data wait for it */
      WaitEvent(POSIXE);
      ClearEvent(POSIXE);

      /* after the wait is not needed to check if data is avaibale on the
       * buffer. The event will be set first after adding some data into it */

      /* try to read nbyte from rxBuf and store it to the user buffer */
      ret = ciaaLibs_circBufGet(&serialDevice->rxBuf,
            buf,
            nbyte);
   }

   return ret;
}

extern int32_t ciaaSerialDevices_write(ciaaDevices_deviceType const * const device, uint8_t const * buf, uint32_t nbyte)
{
   /* get serial device */
   ciaaSerialDevices_deviceType * serialDevice =
      (ciaaSerialDevices_deviceType*) device->layer;
   int32_t ret = 0;
   ciaaLibs_CircBufType * cbuf = &serialDevice->txBuf;
   int32_t head;
   uint32_t space;

   do
   {
      /* read head and space */
      head = cbuf->head;
      space = ciaaLibs_circBufSpace(cbuf, head);

      /* put bytes in the queue */
      ret += ciaaLibs_circBufPut(cbuf, buf, ciaaLibs_min(nbyte-ret, space));

      /* starts the transmission if not already ongoing */
      serialDevice->device->ioctl(
            device->loLayer,
            ciaaPOSIX_IOCTL_STARTTX,
            NULL);

      /* if not all bytes could be stored in the buffer */
      if (ret < nbyte)
      {
         /* increment buffer */
         buf += ret;

         /* set the task to sleep until some data have been send */

         /* get task id for waking up the task later */
         GetTaskID(&serialDevice->taskID);

         /* wait to write all data or for the txConfirmation */
         WaitEvent(POSIXE);
         ClearEvent(POSIXE);
      }
   }
   while (ret < nbyte);

   return ret;
}

extern void ciaaSerialDevices_txConfirmation(ciaaDevices_deviceType const * const device, uint32_t const nbyte)
{
   /* get serial device */
   ciaaSerialDevices_deviceType * serialDevice =
      (ciaaSerialDevices_deviceType*) device->layer;
   uint32_t write = 0;
   ciaaLibs_CircBufType * cbuf = &serialDevice->txBuf;
   uint32_t tail = cbuf->tail;
   uint32_t rawCount = ciaaLibs_circBufRawCount(cbuf, tail);
   uint32_t count = ciaaLibs_circBufCount(cbuf, tail);
   TaskType taskID = serialDevice->taskID;

   /* if some data have to be transmitted */
   if (count > 0)
   {

      /* write data to the driver */
      write = serialDevice->device->write(device->loLayer, ciaaLibs_circBufReadPos(cbuf), rawCount);

      /* update buffer */
      ciaaLibs_circBufUpdateHead(cbuf, write);

      if ( (write == rawCount) && (count > rawCount ) )
      {
         /* re calculate rawCount */
         rawCount = ciaaLibs_circBufRawCount(cbuf, tail);

         /* write more bytes */
         write = serialDevice->device->write(device->loLayer, ciaaLibs_circBufReadPos(cbuf), rawCount);

      }

      if (write > 0)
      {
         /* update buffer */
         ciaaLibs_circBufUpdateHead(cbuf, write);

         if (255 != taskID)
         {
            /* invalidate task id */
            serialDevice->taskID = 255; /* TODO */
            /* set task event */
            SetEvent(taskID, POSIXE);
         }
      }
   }
}

extern void ciaaSerialDevices_rxIndication(ciaaDevices_deviceType const * const device, uint32_t const nbyte)
{
   /* get serial device */
   ciaaSerialDevices_deviceType * serialDevice =
      (ciaaSerialDevices_deviceType*) device->layer;
   ciaaLibs_CircBufType * cbuf = &serialDevice->rxBuf;
   uint32_t head = cbuf->head;
   uint32_t rawSpace = ciaaLibs_circBufRawSpace(cbuf, head);
   uint32_t space = ciaaLibs_circBufSpace(cbuf, head);
   uint32_t read = 0;
   TaskType taskID = serialDevice->taskID;

   read = serialDevice->device->read(device->loLayer, ciaaLibs_circBufWritePos(cbuf), rawSpace);

   /* if rawSpace is full but more space is avaialble */
   if ((read == rawSpace) && (space > rawSpace))
   {
      read += serialDevice->device->read(
            device->loLayer,
            &cbuf->buf[0],
            space - rawSpace);
   }
   else
   {
      if ((read == rawSpace) && (space <= rawSpace))
      {
         /* data may be lost because not place on the receive buffer */
         /* TODO */
      }
      else
      {
         /* read less bytes than provided */
         /* nothing to do */
      }
   }

   /* update tail */
   ciaaLibs_circBufUpdateTail(cbuf, read);

   /* if data has been read */
   if ( (0 < read) && (255 != taskID) )
   {
      /* invalidate task id */
      serialDevice->taskID = 255; /* TODO */
      /* set task event */
      SetEvent(taskID, POSIXE);
   }
}


/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/
