#include "ascii_proto.h"
static const char hexc[]="0123456789ABCDEF";
void ByteToHexStr(uint8_t b, char *out){ out[0]=hexc[(b>>4)&0x0F]; out[1]=hexc[b&0x0F]; }
void BytesToHexStr(uint8_t *bytes, uint16_t len, char *hex_str){
    for(uint16_t i=0;i<len;i++) ByteToHexStr(bytes[i],&hex_str[i*2]);
    hex_str[len*2]='\0';
}
uint8_t HexStrToByte(char h, char l){
    uint8_t v=0;
    if(h>='0'&&h<='9')v=(h-'0')<<4; else if(h>='A'&&h<='F')v=(h-'A'+10)<<4; else if(h>='a'&&h<='f')v=(h-'a'+10)<<4;
    if(l>='0'&&l<='9')v|=(l-'0'); else if(l>='A'&&l<='F')v|=(l-'A'+10); else if(l>='a'&&l<='f')v|=(l-'a'+10);
    return v;
}
uint16_t HexStrToBytes(char *hex_str, uint16_t str_len, uint8_t *bytes){
    uint16_t cnt=str_len/2;
    for(uint16_t i=0;i<cnt;i++) bytes[i]=HexStrToByte(hex_str[i*2],hex_str[i*2+1]);
    return cnt;
}
void SendHexFrame(uint8_t *frame, uint16_t len){
    char hex_str[1024]; BytesToHexStr(frame,len,hex_str); USART1_SendString(hex_str);
}
uint8_t IsHexChar(char c){ return (c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'); }
