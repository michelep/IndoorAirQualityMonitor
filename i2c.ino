// Indoor Air Quality monitor
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty


enum i2c_status   {I2C_WAITING,     // stopped states
                   I2C_TIMEOUT,     //  |
                   I2C_ADDR_NAK,    //  |
                   I2C_DATA_NAK,    //  |
                   I2C_ARB_LOST,    //  |
                   I2C_BUF_OVF,     //  |
                   I2C_NOT_ACQ,     //  |
                   I2C_DMA_ERR,     //  V
                   I2C_SENDING,     // active states
                   I2C_SEND_ADDR,   //  |
                   I2C_RECEIVING,   //  |
                   I2C_SLAVE_TX,    //  |
                   I2C_SLAVE_RX};

bool i2c_status() {
  switch(Wire.status()) {
    case I2C_WAITING:  DEBUG("I2C waiting, no errors"); return true;
    case I2C_ADDR_NAK: DEBUG("Slave addr not acknowledged"); break;
    case I2C_DATA_NAK: DEBUG("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: DEBUG("Bus Error: Arbitration Lost\n"); break;
    case I2C_TIMEOUT:  DEBUG("I2C timeout\n"); break;
    case I2C_BUF_OVF:  DEBUG("I2C buffer overflow\n"); break;
    default:           DEBUG("I2C busy\n"); break;
  }
  return false;
}
