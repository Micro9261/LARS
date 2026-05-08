#ifndef __MFRC522_H__
#define __MFRC522_H__


//Command and status registers
#define MFRC522_COMMAND_REG         0x01
#define MFRC522_COMMIEN_REG         0x02
#define MFRC522_DIVIEN_REG          0x03
#define MFRC522_COMMIRQ_REG         0x04
#define MFRC522_DIVIRQ_REG          0x05
#define MFRC522_ERROR_REG           0x06
#define MFRC522_STATUS1_REG         0x07
#define MFRC522_STATUS2_REG         0x08
#define MFRC522_FIFODATA_REG        0x09
#define MFRC522_FIFOLEVEL_REG       0x0A
#define MFRC522_WATERLEVEL_REG      0x0B
#define MFRC522_CONTROL_REG         0x0C
#define MFRC522_BITFRAMING_REG      0x0D
#define MFRC522_COLLREG_REG         0x0E

//Command registers
#define MFRC522_MODE_REG            0x11
#define MFRC522_TXMODE_REG          0x12
#define MFRC522_RXMODE_REG          0x13
#define MFRC522_TXCONTROL_REG       0x14
#define MFRC522_TXASK_REG           0x15
#define MFRC522_TXSEL_REG           0x16
#define MFRC522_RXSEL_REG           0x17
#define MFRC522_RXTHRESHOLD_REG     0x18
#define MFRC522_DEMOD_REG           0x19
#define MFRC522_MFTX_REG            0x1C
#define MFRC522_MFRX_REG            0x1D
#define MFRC522_SERIALSPEED_REG     0x1F

//Status registers
#define MFRC522_CRCRESULT_REG_MSB   0x21
#define MFRC522_CRCRESULT_REG_LSB   0x22
#define MFRC522_MODWIDTH_REG        0x24
#define MFRC522_RFCFG_REG           0x26
#define MFRC522_GSN_REG             0x27
#define MFRC522_CWGSP_REG           0x28
#define MFRC522_MODGSP_REG          0x29
#define MFRC522_TMODE_REG           0x2A
#define MFRC522_TPRESCALER_REG      0x2B
#define MFRC522_TRELOAD_REG_MSB     0x2C
#define MFRC522_TRELOAD_REG_LSB     0x2D
#define MFRC522_TCOUNTERVAL_REG_MSB 0x2E
#define MFRC522_TCOUNTERVAL_REG_LSB 0x2F

//Test registers
#define MFRC522_TESTSEL1_REG        0x31
#define MFRC522_TESTSEL2_REG        0x32
#define MFRC522_TESTPINEN_REG       0x33
#define MFRC522_TESTPINVALUE_REG    0x34
#define MFRC522_TESTBUS_REG         0x35
#define MFRC522_AUTOTEST_REG        0x36
#define MFRC522_VERSION_REG         0x37
#define MFRC522_ANALOGTEST_REG      0x38
#define MFRC522_TESTDAC1_REG        0x39
#define MFRC522_TESTDAC2_REG        0x3A
#define MFRC522_TESTADC_REG         0x3B

//Command set
#define MFRC522_IDLE                0x00
#define MFRC522_MEM                 0x01
#define MFRC522_GENERATE_RANDOM_ID  0x02
#define MFRC522_CALC_CRC            0x03
#define MFRC522_TRANSMIT            0x04
#define MFRC522_NO_CMD_CHANGE       0x07
#define MFRC522_RECEIVE             0x08
#define MFRC522_TRANSCEIVE          0x0C
#define MFRC522_MFAUTHENT           0x0E
#define MFRC522_SOFTRESET           0x0F

// Mifare_One card command word
# define MFRC522_PICC_REQIDL          0x26               // find the antenna area does not enter hibernation
# define MFRC522_PICC_REQALL          0x52               // find all the cards antenna area
# define MFRC522_PICC_ANTICOLL        0x93               // anti-collision
# define MFRC522_PICC_SElECTTAG       0x93               // election card
# define MFRC522_PICC_AUTHENT1A       0x60               // authentication key A
# define MFRC522_PICC_AUTHENT1B       0x61               // authentication key B
# define MFRC522_PICC_READ            0x30               // Read Block
# define MFRC522_PICC_WRITE           0xA0               // write block
# define MFRC522_PICC_DECREMENT       0xC0               // debit
# define MFRC522_PICC_INCREMENT       0xC1               // recharge
# define MFRC522_PICC_RESTORE         0xC2               // transfer block data to the buffer
# define MFRC522_PICC_TRANSFER        0xB0               // save the data in the buffer
# define MFRC522_PICC_HALT            0x50               // Sleep

#endif /* __MFRC522_H__ */