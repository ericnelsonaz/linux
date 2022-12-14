/*
* Copyright (C) 2017-2018 InvenSense, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/
#define pr_fmt(fmt) "inv_mpu: " fmt

#include "../inv_mpu_iio.h"

/* Constants */
#define SHIFT_RIGHT_4_POSITION				 4
#define SHIFT_LEFT_2_POSITION                2
#define SHIFT_LEFT_4_POSITION                4
#define SHIFT_LEFT_5_POSITION                5
#define SHIFT_LEFT_8_POSITION                8
#define SHIFT_LEFT_12_POSITION               12
#define SHIFT_LEFT_16_POSITION               16

/* Sensor Specific constants */
#define BMP280_SLEEP_MODE                    0x00
#define BMP280_FORCED_MODE                   0x01
#define BMP280_NORMAL_MODE                   0x03
#define BMP280_SOFT_RESET                    0xB6

#define BMP280_DELAYTIME_MS_NONE             0
#define BMP280_DELAYTIME_MS_5                5
#define BMP280_DELAYTIME_MS_6                6
#define BMP280_DELAYTIME_MS_8                8
#define BMP280_DELAYTIME_MS_12               12
#define BMP280_DELAYTIME_MS_22               22
#define BMP280_DELAYTIME_MS_38               38

#define BMP280_OVERSAMPLING_SKIPPED          0x00
#define BMP280_OVERSAMPLING_1X               0x01
#define BMP280_OVERSAMPLING_2X               0x02
#define BMP280_OVERSAMPLING_4X               0x03
#define BMP280_OVERSAMPLING_8X               0x04
#define BMP280_OVERSAMPLING_16X              0x05

#define BMP280_ULTRALOWPOWER_MODE            0x00
#define BMP280_LOWPOWER_MODE	             0x01
#define BMP280_STANDARDRESOLUTION_MODE       0x02
#define BMP280_HIGHRESOLUTION_MODE           0x03
#define BMP280_ULTRAHIGHRESOLUTION_MODE      0x04

#define BMP280_ULTRALOWPOWER_OSRS_P          BMP280_OVERSAMPLING_1X
#define BMP280_ULTRALOWPOWER_OSRS_T          BMP280_OVERSAMPLING_1X

#define BMP280_LOWPOWER_OSRS_P	             BMP280_OVERSAMPLING_2X
#define BMP280_LOWPOWER_OSRS_T	             BMP280_OVERSAMPLING_1X

#define BMP280_STANDARDRESOLUTION_OSRS_P     BMP280_OVERSAMPLING_4X
#define BMP280_STANDARDRESOLUTION_OSRS_T     BMP280_OVERSAMPLING_1X

#define BMP280_HIGHRESOLUTION_OSRS_P         BMP280_OVERSAMPLING_8X
#define BMP280_HIGHRESOLUTION_OSRS_T         BMP280_OVERSAMPLING_1X

#define BMP280_ULTRAHIGHRESOLUTION_OSRS_P    BMP280_OVERSAMPLING_16X
#define BMP280_ULTRAHIGHRESOLUTION_OSRS_T    BMP280_OVERSAMPLING_2X

#define BMP280_FILTERCOEFF_OFF               0x00
#define BMP280_FILTERCOEFF_2                 0x01
#define BMP280_FILTERCOEFF_4                 0x02
#define BMP280_FILTERCOEFF_8                 0x03
#define BMP280_FILTERCOEFF_16                0x04

/*calibration parameters */
#define BMP280_DIG_T1_LSB_REG                0x88
#define BMP280_DIG_T1_MSB_REG                0x89
#define BMP280_DIG_T2_LSB_REG                0x8A
#define BMP280_DIG_T2_MSB_REG                0x8B
#define BMP280_DIG_T3_LSB_REG                0x8C
#define BMP280_DIG_T3_MSB_REG                0x8D
#define BMP280_DIG_P1_LSB_REG                0x8E
#define BMP280_DIG_P1_MSB_REG                0x8F
#define BMP280_DIG_P2_LSB_REG                0x90
#define BMP280_DIG_P2_MSB_REG                0x91
#define BMP280_DIG_P3_LSB_REG                0x92
#define BMP280_DIG_P3_MSB_REG                0x93
#define BMP280_DIG_P4_LSB_REG                0x94
#define BMP280_DIG_P4_MSB_REG                0x95
#define BMP280_DIG_P5_LSB_REG                0x96
#define BMP280_DIG_P5_MSB_REG                0x97
#define BMP280_DIG_P6_LSB_REG                0x98
#define BMP280_DIG_P6_MSB_REG                0x99
#define BMP280_DIG_P7_LSB_REG                0x9A
#define BMP280_DIG_P7_MSB_REG                0x9B
#define BMP280_DIG_P8_LSB_REG                0x9C
#define BMP280_DIG_P8_MSB_REG                0x9D
#define BMP280_DIG_P9_LSB_REG                0x9E
#define BMP280_DIG_P9_MSB_REG                0x9F

#define BMP280_CHIPID_REG                    0xD0	/*Chip ID Register */
#define BMP280_RESET_REG                     0xE0	/*Softreset Register */
#define BMP280_STATUS_REG                    0xF3	/*Status Register */
#define BMP280_CTRLMEAS_REG                  0xF4  /*Ctrl Measure Register */
#define BMP280_CONFIG_REG                    0xF5  /*Configuration Register */
#define BMP280_PRESSURE_MSB_REG              0xF7   /*Pressure MSB Register */
#define BMP280_PRESSURE_LSB_REG              0xF8   /*Pressure LSB Register */
#define BMP280_PRESSURE_XLSB_REG             0xF9  /*Pressure XLSB Register */
#define BMP280_TEMPERATURE_MSB_REG           0xFA  /*Temperature MSB Reg */
#define BMP280_TEMPERATURE_LSB_REG           0xFB  /*Temperature LSB Reg */
#define BMP280_TEMPERATURE_XLSB_REG          0xFC  /*Temperature XLSB Reg */

/* Status Register */
#define BMP280_STATUS_REG_MEASURING__POS           3
#define BMP280_STATUS_REG_MEASURING__MSK           0x08
#define BMP280_STATUS_REG_MEASURING__LEN           1
#define BMP280_STATUS_REG_MEASURING__REG           BMP280_STATUS_REG

#define BMP280_STATUS_REG_IMUPDATE__POS            0
#define BMP280_STATUS_REG_IMUPDATE__MSK            0x01
#define BMP280_STATUS_REG_IMUPDATE__LEN            1
#define BMP280_STATUS_REG_IMUPDATE__REG            BMP280_STATUS_REG

/* Control Measurement Register */
#define BMP280_CTRLMEAS_REG_OSRST__POS             5
#define BMP280_CTRLMEAS_REG_OSRST__MSK             0xE0
#define BMP280_CTRLMEAS_REG_OSRST__LEN             3
#define BMP280_CTRLMEAS_REG_OSRST__REG             BMP280_CTRLMEAS_REG

#define BMP280_CTRLMEAS_REG_OSRSP__POS             2
#define BMP280_CTRLMEAS_REG_OSRSP__MSK             0x1C
#define BMP280_CTRLMEAS_REG_OSRSP__LEN             3
#define BMP280_CTRLMEAS_REG_OSRSP__REG             BMP280_CTRLMEAS_REG

#define BMP280_CTRLMEAS_REG_MODE__POS              0
#define BMP280_CTRLMEAS_REG_MODE__MSK              0x03
#define BMP280_CTRLMEAS_REG_MODE__LEN              2
#define BMP280_CTRLMEAS_REG_MODE__REG              BMP280_CTRLMEAS_REG

/* Configuation Register */
#define BMP280_CONFIG_REG_TSB__POS                 5
#define BMP280_CONFIG_REG_TSB__MSK                 0xE0
#define BMP280_CONFIG_REG_TSB__LEN                 3
#define BMP280_CONFIG_REG_TSB__REG                 BMP280_CONFIG_REG

#define BMP280_CONFIG_REG_FILTER__POS              2
#define BMP280_CONFIG_REG_FILTER__MSK              0x1C
#define BMP280_CONFIG_REG_FILTER__LEN              3
#define BMP280_CONFIG_REG_FILTER__REG              BMP280_CONFIG_REG

#define BMP280_CONFIG_REG_SPI3WEN__POS             0
#define BMP280_CONFIG_REG_SPI3WEN__MSK             0x01
#define BMP280_CONFIG_REG_SPI3WEN__LEN             1
#define BMP280_CONFIG_REG_SPI3WEN__REG             BMP280_CONFIG_REG

/* Data Register */
#define BMP280_PRESSURE_XLSB_REG_DATA__POS         4
#define BMP280_PRESSURE_XLSB_REG_DATA__MSK         0xF0
#define BMP280_PRESSURE_XLSB_REG_DATA__LEN         4
#define BMP280_PRESSURE_XLSB_REG_DATA__REG         BMP280_PRESSURE_XLSB_REG

#define BMP280_TEMPERATURE_XLSB_REG_DATA__POS      4
#define BMP280_TEMPERATURE_XLSB_REG_DATA__MSK      0xF0
#define BMP280_TEMPERATURE_XLSB_REG_DATA__LEN      4
#define BMP280_TEMPERATURE_XLSB_REG_DATA__REG      BMP280_TEMPERATURE_XLSB_REG

#define BMP280_RATE_SCALE  35
#define DATA_BMP280_MIN_READ_TIME            (32 * NSEC_PER_MSEC)
#define BMP280_DATA_BYTES                6
#define DATA_BMP280_ID                   0x58
static int odr_config[] = { 166, 15, 8, 4, 2, 1 };

/** this structure holds all device specific calibration parameters */
struct bmp280_calibration_param_t {
	u32 dig_T1;
	s32 dig_T2;
	s32 dig_T3;
	u32 dig_P1;
	s32 dig_P2;
	s32 dig_P3;
	s32 dig_P4;
	s32 dig_P5;
	s32 dig_P6;
	s32 dig_P7;
	s32 dig_P8;
	s32 dig_P9;

	s32 t_fine;
};
/** BMP280 image registers data structure */
struct bmp280_t {
	struct bmp280_calibration_param_t cal_param;

	u8 chip_id;
	u8 dev_addr;

	u8 waittime;

	u8 osrs_t;
	u8 osrs_p;
};
static struct bmp280_t bmp280;
static bool secondary_resume_state;
static int curr_t_sb;

static int bmp280_get_calib_param(struct inv_mpu_state *st)
{
	u8 d[24];
	int r;

	r = inv_execute_read_secondary(st, 3,
				       st->plat_data.aux_i2c_addr,
				       BMP280_DIG_T1_LSB_REG, 12, d);
	if (r)
		return r;
	r = inv_execute_read_secondary(st, 3,
				       st->plat_data.aux_i2c_addr,
				       BMP280_DIG_T1_LSB_REG + 12, 12, &d[12]);
	if (r)
		return r;

	bmp280.cal_param.dig_T1 = (u16) ((((u16) ((u8) d[1])) <<
					  SHIFT_LEFT_8_POSITION) | d[0]);
	bmp280.cal_param.dig_T2 = (s16) ((((s16) ((s8) d[3])) <<
					  SHIFT_LEFT_8_POSITION) | d[2]);
	bmp280.cal_param.dig_T3 = (s16) ((((s16) ((s8) d[5])) <<
					  SHIFT_LEFT_8_POSITION) | d[4]);
	bmp280.cal_param.dig_P1 = (u16) ((((u16) ((u8) d[7])) <<
					  SHIFT_LEFT_8_POSITION) | d[6]);
	bmp280.cal_param.dig_P2 = (s16) ((((s16) ((s8) d[9])) <<
					  SHIFT_LEFT_8_POSITION) | d[8]);
	bmp280.cal_param.dig_P3 = (s16) ((((s16) ((s8) d[11])) <<
					  SHIFT_LEFT_8_POSITION) | d[10]);
	bmp280.cal_param.dig_P4 = (s16) ((((s16) ((s8) d[13])) <<
					  SHIFT_LEFT_8_POSITION) | d[12]);
	bmp280.cal_param.dig_P5 = (s16) ((((s16) ((s8) d[15])) <<
					  SHIFT_LEFT_8_POSITION) | d[14]);
	bmp280.cal_param.dig_P6 = (s16) ((((s16) ((s8) d[17])) <<
					  SHIFT_LEFT_8_POSITION) | d[16]);
	bmp280.cal_param.dig_P7 = (s16) ((((s16) ((s8) d[19])) <<
					  SHIFT_LEFT_8_POSITION) | d[18]);
	bmp280.cal_param.dig_P8 = (s16) ((((s16) ((s8) d[21])) <<
					  SHIFT_LEFT_8_POSITION) | d[20]);
	bmp280.cal_param.dig_P9 = (s16) ((((s16) ((s8) d[23])) <<
					  SHIFT_LEFT_8_POSITION) | d[22]);

	return 0;
}

static int inv_setup_bmp280(struct inv_mpu_state *st)
{
	int r;
	u8 d[1], addr;

	addr = st->plat_data.aux_i2c_addr;
	/* issue soft reset */
	r = inv_execute_write_secondary(st, 3, addr,
					BMP280_RESET_REG, BMP280_SOFT_RESET);
	if (r)
		return r;

	msleep(100);

	r = inv_execute_read_secondary(st, 3, addr, BMP280_CHIPID_REG, 1, d);
	if (r)
		return r;
	if (d[0] != DATA_BMP280_ID) {
		pr_info("BMP280 not found. Addr:0x%02X, ID:0x%02X (expected:0x%02X)\n",
				addr, d[0], DATA_BMP280_ID);
		return -ENXIO;
	}
	pr_info("BMP280 found. Addr:0x%02X, ID:0x%02X\n", addr, d[0]);

	/* set pressure as ultra high resolution */
	bmp280.osrs_t = BMP280_ULTRAHIGHRESOLUTION_OSRS_T;
	bmp280.osrs_p = BMP280_ULTRAHIGHRESOLUTION_OSRS_P;

	r = bmp280_get_calib_param(st);

	return r;
}

static int inv_check_bmp280_self_test(struct inv_mpu_state *st)
{
	return 0;
}

static int inv_write_bmp280_scale(struct inv_mpu_state *st, int data)
{
	return 0;
}

static int inv_read_bmp280_scale(struct inv_mpu_state *st, int *scale)
{
	return 0;
}

static int inv_resume_bmp280(struct inv_mpu_state *st)
{
	int r, rate, i;
	u8 d, addr;

	rate = st->sensor[SENSOR_PRESSURE].rate;
	i = 0;
	while (rate < odr_config[i])
		i++;

	r = inv_plat_single_write(st, REG_USER_CTRL, st->i2c_dis);
	if (r)
		return r;
	curr_t_sb = i;

	addr = st->plat_data.aux_i2c_addr;
	r = inv_set_bank(st, BANK_SEL_3);
	if (r)
		return r;
	r = inv_plat_single_write(st, REG_I2C_MST_ODR_CONFIG,
				  MIN_MST_ODR_CONFIG);
	if (r)
		return r;
	if (!st->chip_config.compass_enable) {
		r = inv_plat_single_write(st, st->slv_reg[0].ctrl, 0);
		if (r)
			return r;
	}
	if (!st->chip_config.als_enable) {
		r = inv_plat_single_write(st, st->slv_reg[2].ctrl, 0);
		if (r)
			return r;
	}

	/* set IIR filter as 4 */
	d = (i << SHIFT_LEFT_5_POSITION) |
	    (BMP280_FILTERCOEFF_16 << SHIFT_LEFT_2_POSITION);
	r = inv_execute_write_secondary(st, 3, addr,
					BMP280_CONFIG_REG_FILTER__REG, d);
	if (r)
		return r;

	d = (bmp280.osrs_t << SHIFT_LEFT_5_POSITION) +
	    (bmp280.osrs_p << SHIFT_LEFT_2_POSITION) + BMP280_NORMAL_MODE;
	r = inv_execute_write_secondary(st, 3, addr, BMP280_CTRLMEAS_REG, d);
	if (r)
		return r;
	r = inv_set_bank(st, BANK_SEL_3);
	if (r)
		return r;

	/* slave 3 is used to read data from pressure sensor */
	/*read mode */
	r = inv_read_secondary(st, 3, addr, BMP280_PRESSURE_MSB_REG,
			       BMP280_DATA_BYTES);
	if (r)
		return r;
	r = inv_set_bank(st, BANK_SEL_0);

	secondary_resume_state = true;

	return r;
}

static int inv_suspend_bmp280(struct inv_mpu_state *st)
{
	int r;

	if (!secondary_resume_state)
		return 0;
	r = inv_plat_single_write(st, REG_USER_CTRL, st->i2c_dis);
	if (r)
		return r;

	r = inv_set_bank(st, BANK_SEL_3);
	if (r)
		return r;
	r = inv_plat_single_write(st, REG_I2C_MST_ODR_CONFIG,
				  MIN_MST_ODR_CONFIG);
	if (r)
		return r;
	if (!st->chip_config.compass_enable) {
		r = inv_plat_single_write(st, st->slv_reg[0].ctrl, 0);
		if (r)
			return r;
	}
	if (!st->chip_config.als_enable) {
		r = inv_plat_single_write(st, st->slv_reg[2].ctrl, 0);
		if (r)
			return r;
	}
	/* make pressure into sleep mode */
	r = inv_execute_write_secondary(st, 3, st->plat_data.aux_i2c_addr,
					BMP280_CTRLMEAS_REG, 0);
	if (r)
		return r;
	r = inv_set_bank(st, BANK_SEL_3);
	if (r)
		return r;

	/* slave 3 is disabled */
	r = inv_plat_single_write(st, REG_I2C_SLV3_CTRL, 0);
	if (r)
		return r;
	r = inv_set_bank(st, BANK_SEL_0);

	secondary_resume_state = false;

	return r;
}

static s32 bmp280_compensate_T_int32(s32 adc_t)
{
	s32 v_x1_u32r = 0;
	s32 v_x2_u32r = 0;
	s32 temperature = 0;

	v_x1_u32r = ((((adc_t >> 3) - ((s32)
				       bmp280.cal_param.dig_T1 << 1))) *
		     ((s32) bmp280.cal_param.dig_T2)) >> 11;
	v_x2_u32r = (((((adc_t >> 4) -
			((s32) bmp280.cal_param.dig_T1)) * ((adc_t >> 4) -
							    ((s32) bmp280.
							     cal_param.
							     dig_T1))) >> 12) *
		     ((s32) bmp280.cal_param.dig_T3)) >> 14;
	bmp280.cal_param.t_fine = v_x1_u32r + v_x2_u32r;
	temperature = (bmp280.cal_param.t_fine * 5 + 128) >> 8;

	return temperature;
}

static u32 bmp280_compensate_P_int32(s32 adc_p)
{
	s32 v_x1_u32r = 0;
	s32 v_x2_u32r = 0;
	u32 pressure = 0;

	v_x1_u32r = (((s32) bmp280.cal_param.t_fine) >> 1) - (s32) 64000;
	v_x2_u32r = (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 11) *
	    ((s32) bmp280.cal_param.dig_P6);
	v_x2_u32r = v_x2_u32r + ((v_x1_u32r *
				  ((s32) bmp280.cal_param.dig_P5)) << 1);
	v_x2_u32r = (v_x2_u32r >> 2) + (((s32) bmp280.cal_param.dig_P4) << 16);
	v_x1_u32r = (((bmp280.cal_param.dig_P3 * (((v_x1_u32r >> 2) *
						   (v_x1_u32r >> 2)) >> 13)) >>
		      3) +
		     ((((s32) bmp280.cal_param.dig_P2) *
		       v_x1_u32r) >> 1)) >> 18;
	v_x1_u32r =
	    ((((32768 + v_x1_u32r)) * ((s32) bmp280.cal_param.dig_P1)) >> 15);
	/* Avoid exception caused by division by zero */
	if (v_x1_u32r == 0)
		return 0;
	pressure = (((u32) (((s32) 1048576) - adc_p) -
		     (v_x2_u32r >> 12))) * 3125;
	if (pressure < 0x80000000)
		pressure = (pressure << 1) / ((u32) v_x1_u32r);
	else
		pressure = (pressure / (u32) v_x1_u32r) * 2;
	v_x1_u32r = (((s32) bmp280.cal_param.dig_P9) *
		     ((s32) (((pressure >> 3) * (pressure >> 3)) >> 13)))
	    >> 12;
	v_x2_u32r = (((s32) (pressure >> 2)) *
		     ((s32) bmp280.cal_param.dig_P8)) >> 13;
	pressure = (u32) ((s32) pressure +
			  ((v_x1_u32r + v_x2_u32r +
			    bmp280.cal_param.dig_P7) >> 4));

	return pressure;
}

static int inv_bmp280_read_data(struct inv_mpu_state *st, short *o)
{
	int r;
	u8 *d;
	s32 upressure, utemperature;

	d = st->fifo_data;
	/* pressure */
	upressure = (s32) ((((s32) (d[0]))
			    << SHIFT_LEFT_12_POSITION) | (((u32) (d[1]))
							  <<
							  SHIFT_LEFT_4_POSITION)
			   | ((u32) d[2] >> SHIFT_RIGHT_4_POSITION));

	/* Temperature */
	utemperature = (s32) ((((s32) (d[3])) << SHIFT_LEFT_12_POSITION) |
			      (((u32) (d[4])) << SHIFT_LEFT_4_POSITION)
			      | ((u32) d[5] >> SHIFT_RIGHT_4_POSITION));

	bmp280_compensate_T_int32(utemperature);
	r = bmp280_compensate_P_int32(upressure);
	r *= 100; /* Pa x 100 */
	o[0] = 0;
	o[1] = (r >> 16);
	o[2] = (r & 0xffff);

	return 0;
}

static struct inv_mpu_slave slave_bmp280 = {
	.suspend = inv_suspend_bmp280,
	.resume = inv_resume_bmp280,
	.get_scale = inv_read_bmp280_scale,
	.set_scale = inv_write_bmp280_scale,
	.self_test = inv_check_bmp280_self_test,
	.setup = inv_setup_bmp280,
	.read_data = inv_bmp280_read_data,
	.rate_scale = BMP280_RATE_SCALE,
	.min_read_time = DATA_BMP280_MIN_READ_TIME,
};

int inv_mpu_setup_pressure_slave(struct inv_mpu_state *st)
{
	switch (st->plat_data.aux_slave_id) {
	case PRESSURE_ID_BMP280:
		st->slave_pressure = &slave_bmp280;
		break;
	default:
		return -EINVAL;
	}

	return st->slave_pressure->setup(st);
}
