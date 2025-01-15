/* Touchscreen library for 2432W328 Capacitive Touch Controller Chip
 * Supported boards
 *  - 2432W328C
 */

#include <Wire.h>

#define I2C_ADDR_CST820 0x15

#define CYD28_TouchC_SDA 33
#define CYD28_TouchC_SCL 32
#define CYD28_TouchC_INT 21
#define CYD28_TouchC_RST 25

class CYD28_TS_Point {
public:
  CYD28_TS_Point(void) : x(0), y(0), z(0) {}
  CYD28_TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
  int16_t x, y, z; // z is not used, but kept for compatibility of TouchscreenR
};

class CYD28_TouchC {
public:
    constexpr CYD28_TouchC(int16_t w, int16_t h)
        : sizeX_px(w), sizeY_px(h){ }
    bool begin(void);
    bool touched();
    CYD28_TS_Point getPointScaled();
    void setRotation(uint8_t n) { rotation = n % 4; }

private:
    const int16_t sizeX_px;
    const int16_t sizeY_px;
    uint8_t rotation=1;
    uint8_t i2c_read(uint8_t addr);
    uint8_t i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length);
    void i2c_write(uint8_t addr, uint8_t data);
    CYD28_TS_Point convertRawXY(int16_t x, int16_t y);
};