#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version_stmbl.h"
#include "ade.h"
#include "config.h"
#include <string.h>
#include <math.h>

#if 0

extern union ade_burst_rx_t foobar;
extern uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];

void influx_task(void){
    int ret;

    while(1){
        extern struct ade_float_t ade_f;
        int influx_data_len = snprintf(influx_data, sizeof(influx_data), "%s,%s=%s avrms=%.2f,bvrms=%.2f,cvrms=%.2f,airms=%.2f,birms=%.2f,cirms=%.2f,nirms=%.2f,avthd=%.3f,bvthd=%3f,cvthd=%3f,aithd=%3f,bithd=%3f,cithd=%3f,apf=%3f,bpf=%3f,cpf=%3f,ahz=%.4f,bhz=%.4f,chz=%.4f,isum=%.4f,awatt=%.4f,bwatt=%.4f,cwatt=%.4f,afwatt=%.4f,bfwatt=%.4f,cfwatt=%.4f,avar=%.4f,bvar=%.4f,cvar=%.4f,afvar=%.4f,bfvar=%.4f,cfvar=%.4f,ava=%.4f,bva=%.4f,cva=%.4f,afva=%.4f,bfva=%.4f,cfva=%.4f\r\n\r\n",
        influx_measurement,
        influx_tag,
        influx_tag_value,
                ade_f.avrms1012, ade_f.bvrms1012, ade_f.cvrms1012, ade_f.airms1012, ade_f.birms1012, ade_f.cirms1012, ade_f.nirms1012, ade_f.avthd, ade_f.bvthd, ade_f.cvthd, ade_f.aithd, ade_f.bithd, ade_f.cithd, ade_f.apf, ade_f.bpf, ade_f.cpf, ade_f.ahz, ade_f.bhz, ade_f.chz,ade_f.isumrms,ade_f.awatt, ade_f.bwatt, ade_f.cwatt, ade_f.afwatt, ade_f.bfwatt, ade_f.cfwatt,ade_f.avar, ade_f.bvar, ade_f.cvar, ade_f.afvar, ade_f.bfvar, ade_f.cfvar,ade_f.ava, ade_f.bva, ade_f.cva, ade_f.afva, ade_f.bfva, ade_f.cfva);
        influx_request[0] = '\0';
        influx_request_len = snprintf(influx_request,sizeof(influx_request),"POST /write?db=%s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: lwip\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n%s",influx_db,influx_ip,influx_port,influx_data_len-4,influx_data);
        influx_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
        //SEGGER_RTT_printf(0, "%s\n",influx_request);
        //SEGGER_RTT_printf(0, "size: %d\n",influx_request_len);
        ret = lwip_connect(influx_socket, (struct sockaddr*)&influx_addr, sizeof(influx_addr));
        //SEGGER_RTT_printf(0, "connect %d %d\n",ret,errno);
        ret = lwip_write(influx_socket, influx_request, influx_request_len);
        //SEGGER_RTT_printf(0, "write %d %d\n",ret,errno);
        ret = lwip_close(influx_socket);
        //SEGGER_RTT_printf(0, "close %d %d\n",ret,errno);
        vTaskDelay(500);
    }
}

#endif

extern struct ade_float_t ade_f;

int32_t dmr_rx, dmr_tx, dmr_cc;

typedef enum {
	SYNC_BS_SOURCED_VOICE,
	SYNC_BS_SOURCED_DATA,
	SYNC_MS_SOURCED_VOICE,
	SYNC_MS_SOURCED_DATA,
	SYNC_MS_SOURCED_RC,
	SYNC_DIRECT_VOICE_TS1,
	SYNC_DIRECT_DATA_TS1,
	SYNC_DIRECT_VOICE_TS2,
	SYNC_DIRECT_DATA_TS2,
	SYNC_RESERVED,
} sync_t;
static const uint8_t sync_pattern[][7] = {
	{ 0x07, 0x55, 0xfd, 0x7d, 0xf7, 0x5f, 0x70 },  // 0  BS voice 
	{ 0x0d, 0xff, 0x57, 0xd7, 0x5d, 0xf5, 0xd0 },  // 1  BS data  
	{ 0x07, 0xf7, 0xd5, 0xdd, 0x57, 0xdf, 0xd0 },  // 2  MS voice 
	{ 0x0d, 0x5d, 0x7f, 0x77, 0xfd, 0x75, 0x70 },  // 3  MS data  
	{ 0x07, 0x7d, 0x55, 0xf7, 0xdf, 0xd7, 0x70 },  // 4  MS RC    
	{ 0x05, 0xd5, 0x77, 0xf7, 0x75, 0x7f, 0xf0 },  // 5  D1 voice 
	{ 0x0f, 0x7f, 0xdd, 0x5d, 0xdf, 0xd5, 0x50 },  // 6  D1 data  
	{ 0x07, 0xdf, 0xfd, 0x5f, 0x55, 0xd5, 0xf0 },  // 7  D2 voice 
	{ 0x0d, 0x75, 0x57, 0xf5, 0xff, 0x7f, 0x50 },  // 8  D2 data  
	{ 0x0d, 0xd7, 0xff, 0x5d, 0x75, 0x7d, 0xd0 },  // 9  Reserved 
};

enum {
	SLOTTYPE_PI_HEADER,
	SLOTTYPE_VOICE_LC_HEADER,
	SLOTTYPE_TERMINATOR_WITH_LC,
	SLOTTYPE_CSBK,
	SLOTTYPE_MBC_HEADER,
	SLOTTYPE_MBC_CONTINUATION,
	SLOTTYPE_DATA_HEADER,
	SLOTTYPE_RATE_12_DATA,
	SLOTTYPE_RATE_34_DATA,
	SLOTTYPE_IDLE,
	SLOTTYPE_RATE_1_DATA,
	SLOTTYPE_UNIFIED_SINGLE_BLOCK_DATA,
};

typedef uint32_t bptc[13];

//

static uint8_t mbuf[5*12]; // message buffer
static uint32_t mbuflen;

static uint8_t dbuf[5*37]; // encoded message buffer
static uint32_t dbuflen;

static UART_HandleTypeDef dmr_uart;

static uint16_t calc_crc_ccitt(uint8_t *data, int16_t len)
{
	int16_t j;
	uint16_t crc, xor_flag;
	uint8_t v, i;
	
	crc = 0;
	for (j = 0; j < len; j++) {
		v = 0x80;
		for (i = 0; i < 8; i++) {
			xor_flag = crc & 0x8000;
			
			crc <<= 1;
			
			if (data[j] & v)
				crc++;
			
			if (xor_flag)
				crc ^= 0x1021;
			
			v >>= 1;
		}
	}
	
	for (i = 0; i < 16; i++) {
		xor_flag = crc & 0x8000;
		
		crc <<= 1;
		
		if (xor_flag)
			crc ^= 0x1021;
	}
	
	return ~crc;
}

static uint32_t get_slot_type_bits(uint8_t type, uint8_t cc)
{
	int16_t i;
	uint32_t slottype;
	static const uint32_t golay_gen[] = {
		0b100011101011, // 0 0 0 0 0 0 0 1
		0b100100111110, // 0 0 0 0 0 0 1 0
		0b101010010111, // 0 0 0 0 0 1 0 0
		0b110111000110, // 0 0 0 0 1 0 0 0
		0b001101100111, // 0 0 0 1 0 0 0 0
		0b011011001101, // 0 0 1 0 0 0 0 0
		0b110110011001, // 0 1 0 0 0 0 0 0
		0b001111011010, // 1 0 0 0 0 0 0 0
	};
	
	slottype = 0;
	slottype |= cc   << (4+12);
	slottype |= type << (  12);
	
	for (i = 0; i < 8; i++) {
		slottype ^=  ((slottype&(1<<(12+i)))?1:0) * golay_gen[i];
	}
	
	return slottype;
}

static void bptc_init(bptc p)
{
	int16_t j;
	
	for (j = 0; j < 13; j++) {
		p[j] = 0;
	}
}

static void bptc_fill(bptc p, uint8_t *d)
{
	//    0  1  2  3  4  5  6  7  8  9  a  b
	// 96 88 80 72 64 56 48 40 32 24 16 08 00
	p[ 0] =                 (d[ 0] <<  4)                    ; // -3-     8
	p[ 1] =                 (d[ 1] <<  7) | ((d[ 2] >> 5)<<4); //     8 + 3
	p[ 2] =                 (d[ 2] << 10) | ((d[ 3] >> 2)<<4); //     5 + 6
	p[ 3] = (d[ 3] << 13) | (d[ 4] <<  5) | ((d[ 5] >> 7)<<4); // 2 + 8 + 1
	p[ 4] =                 (d[ 5] <<  8) | ((d[ 6] >> 4)<<4); //     7 + 4
	p[ 5] =                 (d[ 6] << 11) | ((d[ 7] >> 1)<<4); //     4 + 7
	p[ 6] = (d[ 7] << 14) | (d[ 8] <<  6) | ((d[ 9] >> 6)<<4); // 1 + 8 + 2
	p[ 7] =                 (d[ 9] <<  9) | ((d[10] >> 3)<<4); //     6 + 5
	p[ 8] = (d[10] << 12) | (d[11] <<  4)                    ; // 3 + 8
}

static void bptc_parity(bptc p)
{
	int16_t j, i;
	static const uint32_t hamming_gen[] = {
		0b0011, // 0 0 0 0 0 0 0 0 0 0 1
		0b0110, // 0 0 0 0 0 0 0 0 0 1 0
		0b1100, // 0 0 0 0 0 0 0 0 1 0 0
		0b1011, // 0 0 0 0 0 0 0 1 0 0 0
		0b0101, // 0 0 0 0 0 0 1 0 0 0 0
		0b1010, // 0 0 0 0 0 1 0 0 0 0 0
		0b0111, // 0 0 0 0 1 0 0 0 0 0 0
		0b1110, // 0 0 0 1 0 0 0 0 0 0 0
		0b1111, // 0 0 1 0 0 0 0 0 0 0 0
		0b1101, // 0 1 0 0 0 0 0 0 0 0 0
		0b1001, // 1 0 0 0 0 0 0 0 0 0 0
	};
	
	for (j = 0; j <  9; j++) {
		for (i = 0; i < 11; i++) {
			p[j] ^=  ((p[j]&(1<<(4+i)))?1:0) * hamming_gen[i];
		}
	}
	
	// col parity
	p[ 9] = p[0] ^ p[1] ^   0  ^ p[3] ^   0  ^ p[5] ^ p[6] ^   0  ^   0 ;
	p[10] = p[0] ^ p[1] ^ p[2] ^   0  ^ p[4] ^   0  ^ p[6] ^ p[7] ^   0 ;
	p[11] = p[0] ^ p[1] ^ p[2] ^ p[3] ^   0  ^ p[5] ^   0  ^ p[7] ^ p[8];
	p[12] = p[0] ^   0  ^ p[2] ^   0  ^ p[4] ^ p[5] ^   0  ^   0  ^ p[8];
}

static void dmr_usbd_encode(uint8_t colorcode, sync_t sync, const uint8_t msg[10])
{
	int16_t i;
	uint16_t csum;
	uint32_t slottype;
	bptc matrix;
	
	// clear buffer
	mbuflen = 12;
	dbuflen = 37;
	
	// copy message
	memcpy(mbuf, msg, 10);
	
	// calculate crc over header
	csum = calc_crc_ccitt(mbuf, 10) ^ 0x3333;
	mbuf[10] = (csum>> 8)&0xff;
	mbuf[11] = (csum>> 0)&0xff;
	
	//////////////////////////////////////////////////////////////////////////////////
	
	// MMDVM command
	dbuf[0] = 0xe0; // frame-start
	dbuf[1] = 0x25; // len
	dbuf[2] = 0x1a; // command: MMDVM_DMR_DATA2
	dbuf[3] = 0x00; // x
	
	// sync word
	for (i = 0; i < 7; i++) {
		dbuf[17 + i] = sync_pattern[sync][i];
	}
	
	// slot type
	slottype = get_slot_type_bits(SLOTTYPE_UNIFIED_SINGLE_BLOCK_DATA, colorcode);
	dbuf[16] |= (slottype>>14) & 0x3f;
	dbuf[17] |= (slottype>> 6) & 0xf0;
	dbuf[23] |= (slottype>> 6) & 0x0f;
	dbuf[24] |= (slottype<< 2) & 0xfc;
	
	// BPTC(196,96)
	bptc_init(matrix);
	bptc_fill(matrix, mbuf);
	bptc_parity(matrix);
	
	// interleave
	for (i = 0; i < 195; i++) {
		uint8_t bit;
		int16_t ii = ((i+1)*181) % 196;
		
		bit = !!(matrix[i/15] & (1<<(14-(i%15))));
		//printf("%3d -> %3d\n", i, ii);
		//printf("%3d %3d %d\n", i+1, ii, bit);
		ii += (ii>=98) ? 68 : 0;
		
		//printf("%d", bit);
		dbuf[4 + ii/8] |= bit << (7-(ii%8));
	}
	//printf("\n");
}

void dmr_init(uint32_t freq_rx, uint32_t freq_tx, uint8_t color_code)
{
	uint32_t freq_ps = freq_tx;
	
	dbuflen = 0;
	
	// 0x04 h MMDVM_SET_FREQ     setFreq() -> ack/nack
	// e0 11 04 00 10 ec d5 19 10 ec d5 19 ff 40 0e cf 19
	dbuf[dbuflen++] = 0xE0; // frame-start
	dbuf[dbuflen++] = 0x11; // len
	dbuf[dbuflen++] = 0x04; // set-freq
	dbuf[dbuflen++] = 0x00; // unused
	dbuf[dbuflen++] = (freq_rx>> 0)&0xff; // freq rx [Hz, int, LE]
	dbuf[dbuflen++] = (freq_rx>> 8)&0xff;
	dbuf[dbuflen++] = (freq_rx>>16)&0xff;
	dbuf[dbuflen++] = (freq_rx>>24)&0xff;
	dbuf[dbuflen++] = (freq_tx>> 0)&0xff; // freq tx [Hz, int, LE]
	dbuf[dbuflen++] = (freq_tx>> 8)&0xff;
	dbuf[dbuflen++] = (freq_tx>>16)&0xff;
	dbuf[dbuflen++] = (freq_tx>>24)&0xff;
	dbuf[dbuflen++] = 0xFF;               // rf-power
	dbuf[dbuflen++] = (freq_ps>> 0)&0xff; // freq pocsag [Hz, int, LE]
	dbuf[dbuflen++] = (freq_ps>> 8)&0xff;
	dbuf[dbuflen++] = (freq_ps>>16)&0xff;
	dbuf[dbuflen++] = (freq_ps>>24)&0xff;
	
	// 0x02 h MMDVM_SET_CONFIG   setConfig(..) -> ack/nack
	// e0 15 02 92 02 0a 00 80 80 01 00 80 80 80 80 80 80 80 80 04 80
	dbuf[dbuflen++] = 0xE0; // frame-start
	dbuf[dbuflen++] = 0x15; // len
	dbuf[dbuflen++] = 0x02; // set-config
	dbuf[dbuflen++] = 0x92; //  flags, 10010010 | 80 simplex, 10 debug, 02 tx-invert
	dbuf[dbuflen++] = 0x02; //  mode-enable  == DMR
	dbuf[dbuflen++] = 0x0a; //  tx-delay 10ms
	dbuf[dbuflen++] = 0x00; //  mode: idle
	dbuf[dbuflen++] = 0x80; //  rx-level
	dbuf[dbuflen++] = 0x80; //  cwid-tx-level
	dbuf[dbuflen++] = color_code; //  dmr-color-code
	dbuf[dbuflen++] = 0x00; //  dmr-delay [duplex only]
	dbuf[dbuflen++] = 0x80; //  [used to be osc-offset]
	dbuf[dbuflen++] = 0x80; //  tx-level dstar
	dbuf[dbuflen++] = 0x80; //  tx-level dmr
	dbuf[dbuflen++] = 0x80; //  tx-level ysf
	dbuf[dbuflen++] = 0x80; //  tx-level p25
	dbuf[dbuflen++] = 0x80; //  txDCOoffset
	dbuf[dbuflen++] = 0x80; //  rxDVOoffset
	dbuf[dbuflen++] = 0x80; //  tx-level nxdn
	dbuf[dbuflen++] = 0x04; //  ysfTXhang
	dbuf[dbuflen++] = 0x80; //  tx-level pocsag
}

static void dmr_wakeup(void)
{
	dbuflen = 0;
	
	// e0 03 01
	// e0 04 03 02
	// e0 03 01
	dbuf[dbuflen++] = 0xE0; // frame-start
	dbuf[dbuflen++] = 0x03; // len
	dbuf[dbuflen++] = 0x01; // get-status
	
	dbuf[dbuflen++] = 0xE0; // frame-start
	dbuf[dbuflen++] = 0x04; // len
	dbuf[dbuflen++] = 0x03; // set-mode
	dbuf[dbuflen++] = 0x02; // DMR
	
	dbuf[dbuflen++] = 0xE0; // frame-start
	dbuf[dbuflen++] = 0x03; // len
	dbuf[dbuflen++] = 0x01; // get-status
}

static void dmr_put_dbuf(void)
{
	HAL_UART_Transmit(&dmr_uart, dbuf, dbuflen, HAL_MAX_DELAY);
	dbuflen=0;
}

static void dmr_uart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	dmr_uart.Instance = USART2;
	dmr_uart.Init.BaudRate = 115200;
	dmr_uart.Init.WordLength = UART_WORDLENGTH_8B;
	dmr_uart.Init.StopBits = UART_STOPBITS_1;
	dmr_uart.Init.Parity = UART_PARITY_NONE;
	dmr_uart.Init.Mode = UART_MODE_TX_RX;
	dmr_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	dmr_uart.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&dmr_uart);
}

void dmr_task(void) {
	const uint8_t msg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	
        vTaskDelay(5000);
	
	dmr_uart_init();
	dmr_init(dmr_rx, dmr_tx, dmr_cc);
	dmr_put_dbuf();
	
	while (1) {
		dmr_wakeup();
		dmr_put_dbuf();
		dmr_usbd_encode(dmr_cc, SYNC_MS_SOURCED_DATA, msg);
		dmr_put_dbuf();
		
        	vTaskDelay(500);
	}
}

void dmr_task_init(void) {
	float freq;
	
	config_get_float("dmr_tx", &freq);
	if (freq == 0)
		return;
	dmr_tx = freq * 1e6 + 0.5;
	
	config_get_float("dmr_rx", &freq);
	if (freq == 0)
		return;
	dmr_rx = freq * 1e6 + 0.5;
	
	config_get_int("dmr_cc", &dmr_cc);
	if (dmr_cc == 0)
		return;
	
	xTaskCreate((TaskFunction_t)dmr_task, "MMDVM task", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}

//EOF

