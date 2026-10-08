/* License-Identifier: GPL-2.0 */
/*
 * Definitions for qmi8658 imu chip.
 */
#ifndef __QMI8658_H
#define __QMI8658_H

#include <linux/ioctl.h>

#define SENSOR_CHIP_ID_QMI8658      0x05
#define QMI8658_PRECISION		16

/* QMI8658 registers */
#define QMI8658_REG_WHO_AM_I        0x00
#define QMI8658_REG_VERSION         0x01
#define QMI8658_REG_CTRL1           0x02
#define QMI8658_REG_CTRL2           0x03
#define QMI8658_REG_CTRL3           0x04
#define QMI8658_REG_CTRL4           0x05
#define QMI8658_REG_CTRL5           0x06
#define QMI8658_REG_CTRL6           0x07
#define QMI8658_REG_CTRL7           0x08
#define QMI8658_REG_CTRL8           0x09
#define QMI8658_REG_CTRL9           0x0a
#define QMI8658_REG_CAL1_L          0x0b
#define QMI8658_REG_CAL1_H          0x0c
#define QMI8658_REG_CAL2_L          0x0d
#define QMI8658_REG_CAL2_H          0x0e
#define QMI8658_REG_CAL3_L          0x0f
#define QMI8658_REG_CAL3_H          0x10
#define QMI8658_REG_CAL4_L          0x11
#define QMI8658_REG_CAL4_H          0x12
#define QMI8658_REG_FIFO_WM         0x13
#define QMI8658_REG_FIFO_CTRL       0x14
#define QMI8658_REG_FIFO_COUNT      0x15
#define QMI8658_REG_FIFO_STATUS     0x16
#define QMI8658_REG_FIFO_DATA       0x17
#define QMI8658_REG_STATUSINT       0x2d
#define QMI8658_REG_STATUS0         0x2e
#define QMI8658_REG_STATUS1         0x2f
#define QMI8658_REG_RESET           0x60
#define QMI8658_REG_ACC_XOUT_LSB    0x35
#define QMI8658_REG_GYR_XOUT_LSB    0x3B

/* QMI8658 Ctrl9 command*/
#define QMI8658_CRTL9_NOP           0x00
#define QMI8658_CRTL9_RST_FIFO      0x04
#define QMI8658_CRTL9_REQ_FIFO      0x05
#define QMI8658_CRTL9_REQ_MOTION    0x0e
#define QMI8658_CRTL9_CLC_SEN       0xa2

/*QMI8658 acc range*/
#define QMI8658_ACC_RANGE_2G        (0x00<<4) /*!< \brief +/- 2g range */
#define QMI8658_ACC_RANGE_4G        (0x01<<4) /*!< \brief +/- 4g range */
#define QMI8658_ACC_RANGE_8G        (0x02<<4) /*!< \brief +/- 8g range */
#define QMI8658_ACC_RANGE_16G       (0x03<<4)/*!< \brief +/- 16g range */
/*QMI8658 gyr range*/
#define QMI8658_GYR_RANGE_16DPS     (0x00<<4) /*!< \brief +-16 degrees per second. */
#define QMI8658_GYR_RANGE_32DPS     (0x01<<4) /*!< \brief +-32 degrees per second. */
#define QMI8658_GYR_RANGE_64DPS     (0x02<<4) /*!< \brief +-64 degrees per second. */
#define QMI8658_GYR_RANGE_128DPS    (0x03<<4) /*!< \brief +-128 degrees per second. */
#define QMI8658_GYR_RANGE_256DPS    (0x04<<4) /*!< \brief +-256 degrees per second. */
#define QMI8658_GYR_RANGE_512DPS    (0x05<<4) /*!< \brief +-512 degrees per second. */
#define QMI8658_GYR_RANGE_1024DPS   (0x06<<4) /*!< \brief +-1024 degrees per second. */
#define QMI8658_GYR_RANGE_2048DPS   (0x07<<4) /*!< \brief +-2048 degrees per second.*/
/*QMI8658 a,g odr*/
#define QMI8658_ODR_30HZ_REG_VALUE      0x08  /*!< 1.5625 Hz (640 ms)*/
#define QMI8658_ODR_59HZ_REG_VALUE      0x07  /*!< 3.125 Hz (320 ms)*/
#define QMI8658_ODR_118HZ_REG_VALUE     0x06  /*!< 6.25 Hz (160 ms)*/
#define QMI8658_ODR_235HZ_REG_VALUE     0x05  /*!< 12.5 Hz (80 ms)*/
#define QMI8658_ODR_470HZ_REG_VALUE     0x04  /*!< 25 Hz (40 ms)*/


/*function cfg default*/
#define QMI8658_ADDR_AI_BE          0x7C//0x60
#define QMI8658_SW_RESET            0xB0
#define QMI8658_MOTION_HS           0x80
#define QMI8658_ACC_RANGE_DEF       0x20  //±8g
#define QMI8658_GYRO_RANGE_DEF      0x70  //±2048dps
#define QMI8658_WM_DEF              0x20  //watermark 32
#define QMI8658_FIFO_CTRL_STBY      0x00  //fifo standby
#define QMI8658_FIFO_CTRL_DEF       0x0d  //64 samples,fifo mode
#define QMI8658_DATE_READY_OFF      0x20  //disable data ready int
#define QMI8658_FIFO_MAX_LENTH      0x40

#endif

