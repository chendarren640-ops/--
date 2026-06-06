#include "oled_drv.h"
#include "OLEDfont.h"

static uint8_t GRAM[OLED_W][4];

/* I2C软模拟 (PB8=SCL, PB9=SDA) */
#define SCL(x)  gpio_bit_write(GPIOB,GPIO_PIN_8,(x))
#define SDA(x)  gpio_bit_write(GPIOB,GPIO_PIN_9,(x))
#define SDA_RD  gpio_input_bit_get(GPIOB,GPIO_PIN_9)

static void i2c_dly(void){ volatile uint16_t t=120; while(t--); }

static void i2c_start(void) {
    gpio_mode_set(GPIOB,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,GPIO_PIN_9);
    gpio_output_options_set(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,GPIO_PIN_9);
    SDA(1);SCL(1);i2c_dly();SDA(0);i2c_dly();SCL(0);i2c_dly();
}
static void i2c_stop(void) {
    gpio_mode_set(GPIOB,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,GPIO_PIN_9);
    gpio_output_options_set(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,GPIO_PIN_9);
    SDA(0);SCL(1);i2c_dly();SDA(1);
}
static uint8_t i2c_wait_ack(void) {
    uint8_t to=0; SDA(1);i2c_dly();SCL(1);i2c_dly();
    gpio_mode_set(GPIOB,GPIO_MODE_INPUT,GPIO_PUPD_PULLUP,GPIO_PIN_9);
    while(SDA_RD){if(++to>254){i2c_stop();return 1;}}
    SCL(0);i2c_dly();
    gpio_mode_set(GPIOB,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,GPIO_PIN_9);
    gpio_output_options_set(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,GPIO_PIN_9);
    return 0;
}
static void i2c_send(uint8_t d) {
    for(uint8_t i=0;i<8;i++){SCL(0);if(d&0x80)SDA(1);else SDA(0);i2c_dly();SCL(1);i2c_dly();SCL(0);d<<=1;}
}
static void oled_wr(uint8_t d,uint8_t cmd) {
    i2c_start();i2c_send(0x78);i2c_wait_ack();
    i2c_send(cmd?0x40:0x00);i2c_wait_ack();
    i2c_send(d);i2c_wait_ack();i2c_stop();
}

void OLED_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(GPIOB,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,GPIO_PIN_8|GPIO_PIN_9);
    gpio_output_options_set(GPIOB,GPIO_OTYPE_OD,GPIO_OSPEED_50MHZ,GPIO_PIN_8|GPIO_PIN_9);
    SCL(1);SDA(1);delay_1ms(500);
    uint8_t seq[]={0xAE,0x00,0x10,0x40,0x81,0xCF,0xA1,0xC8,0xA6,0xA8,0x1F,
        0xD3,0x00,0xD5,0x80,0xD9,0xF1,0xDA,0x00,0xDB,0x40,0x20,0x02,0x8D,0x14,0xA4,0xA6,0xAF};
    for(uint8_t i=0;i<sizeof(seq);i++) oled_wr(seq[i],0);
    OLED_Clear();
}

void OLED_Clear(void){memset(GRAM,0,sizeof(GRAM));OLED_Refresh();}

void OLED_Refresh(void){
    for(uint8_t p=0;p<4;p++){oled_wr(0xB0+p,0);oled_wr(0x00,0);oled_wr(0x10,0);for(uint8_t c=0;c<128;c++)oled_wr(GRAM[c][p],1);}
}

void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size){
    uint8_t y0=y, sz2=(size/8+((size%8)?1:0))*(size/2);
    chr-=' ';if(size==12){for(uint8_t i=0;i<sz2;i++){uint8_t tmp=asc2_1206[chr][i];for(uint8_t m=0;m<8;m++){if(tmp&0x80){GRAM[x][y/8]|=1<<(y%8);}else{GRAM[x][y/8]&=~(1<<(y%8));}tmp<<=1;y++;if((y-y0)==size){y=y0;x++;break;}}}}
    else if(size==16){for(uint8_t i=0;i<sz2;i++){uint8_t tmp=asc2_1608[chr][i];for(uint8_t m=0;m<8;m++){if(tmp&0x80){GRAM[x][y/8]|=1<<(y%8);}else{GRAM[x][y/8]&=~(1<<(y%8));}tmp<<=1;y++;if((y-y0)==size){y=y0;x++;break;}}}}
}

void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *str,uint8_t size){
    while((*str>=' ')&&(*str<='~')){OLED_ShowChar(x,y,*str,size);x+=size/2;if(x>OLED_W-size){x=0;y+=2;}str++;}
}

void OLED_ShowLine1(uint8_t *str){OLED_Clear();OLED_ShowString(0,0,str,16);OLED_Refresh();}
void OLED_ShowLine2(uint8_t *str){OLED_ShowString(0,16,str,16);OLED_Refresh();}
