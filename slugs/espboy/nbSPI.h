/*
    Interrupt based SPI writeBytes() function for ESP8266
    -----------------------------------------------------
    Use nbSPI_writeBytes(data, size) instead of SPI.writeBytes(data, size).
    nbSPI_writeBytes() will return almost immediatly and NOT block the CPU
    until all data is sent.
    Sending 80 Bytes via nbSPI_writeBytes blocks the CPU for 10µs (6.6µs + 3.5µs).
    Sending 80 Bytes @ 20MHz via SPI.writeBytes blocks the CPU for 40µs.
    Sending 80 Bytes @ 8MHz via SPI.writeBytes blocks the CPU for 88µs.
*/

#pragma once
#include <SPI.h>

// Set an IO to measure timing
#ifdef NBSPI_TDBG_IO
    #if NBSPI_TDBG_IO < 16
        #define NBSPI_TDBG_HIGH GPOS = (1<<NBSPI_TDBG_IO) // 0.1 µs
        #define NBSPI_TDBG_LOW GPOC = (1<<NBSPI_TDBG_IO) // 0.1 µs
    #else
        #error "Use IO 0-15!"
    #endif
#else
    #define NBSPI_TDBG_HIGH
    #define NBSPI_TDBG_LOW
#endif

uint16_t _nbspi_size = 0;
uint32_t * _nbspi_data;
boolean _nbspi_isbusy = false;

//void nbSPI_writeBytes(uint32_t *data, uint16_t size);
boolean nbSPI_isBusy();
//void nbSPI_writeChunk();
void nbSPI_writeBufferChunk();
//void nbSPI_ISR();
void nbSPI_BUFFER_ISR();

void nbSPI_writeBuffer(uint32_t *data, uint32_t n);


boolean nbSPI_isBusy() {
    if( _nbspi_isbusy ) return true; // true while not all data put into buffer
    return (SPI1CMD & SPIBUSY); // true while SPI sends data
}

void nbSPI_writeBuffer(uint32_t *data, uint32_t n){
    _nbspi_isbusy = true;
    _nbspi_size = n;
    _nbspi_data = data ;
    SPI0S &= ~(0x1F); // Disable and clear all interrupts on SPI0
    SPI1S &= ~(0x1F); // Disable and clear all interrupts on SPI1
    ETS_SPI_INTR_ATTACH(nbSPI_BUFFER_ISR, NULL); // Set Interrupt handler
    ETS_SPI_INTR_ENABLE(); // Enable SPI Interrupts
    nbSPI_writeBufferChunk(); // Write first chunk of data
}

// This will send 64 bytes via SPI
IRAM_ATTR void inline nbSPI_writeBufferChunk() {    
    SPI1U1 = ((SPI1U1 & (u32)( ~(SPIMMOSI << SPILMOSI) ) ) | ( (u32)((64 * 8) - 1) << SPILMOSI));

    // memcpy( (void*)&SPI1W0, (const void *)_nbspi_data, 16*4 ) ;
    SPI1W0 = _nbspi_data[0];
    SPI1W1 = _nbspi_data[1];
    SPI1W2 = _nbspi_data[2];
    SPI1W3 = _nbspi_data[3];
    SPI1W4 = _nbspi_data[4];
    SPI1W5 = _nbspi_data[5];
    SPI1W6 = _nbspi_data[6];
    SPI1W7 = _nbspi_data[7];
    SPI1W8 = _nbspi_data[8];
    SPI1W9 = _nbspi_data[9];
    SPI1W10 = _nbspi_data[10];
    SPI1W11 = _nbspi_data[11];
    SPI1W12 = _nbspi_data[12];
    SPI1W13 = _nbspi_data[13];
    SPI1W14 = _nbspi_data[14];
    SPI1W15 = _nbspi_data[15];

    _nbspi_data += 16 ;

    __sync_synchronize();

    if( --_nbspi_size ) {
        // There is more data to send after this one, enable Interrupt
        SPI1S |= SPISTRIE; // Enable Transmission End interrupt on SPI1, SPI_TRANS_DONE_EN
        SPI1CMD |= SPIBUSY;
    } else {
        // All data is sent after this one
        ETS_SPI_INTR_DISABLE(); // Disable SPI Interrupts
        SPI1CMD |= SPIBUSY;
        _nbspi_isbusy = false;
    }
}

// Interrupt Handler gets called when SPI finished sending data
IRAM_ATTR void nbSPI_BUFFER_ISR() {
    if(SPIIR & (1 << SPII0)) {
        // SPI0 Interrupt
        SPI0S &= ~(0x1F); // Disable and clear all interrupts on SPI0
    }
    if(SPIIR & (1 << SPII1)) {
        // SPI1 Interrupt
        SPI1S &= ~(0x1F); // Disable and clear all interrupts on SPI1
        nbSPI_writeBufferChunk(); // Write remaining data
    }
}
