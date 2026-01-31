#include "protocol.h"



PDU *mkPDU(uint uiMsgLen)
{
    uint uiPDULen = sizeof(PDU)+uiMsgLen;   //caMsg[] 弹性 发送消息长度
    PDU *pdu  = (PDU*)malloc(uiPDULen);
    if(nullptr == pdu){
        exit(EXIT_FAILURE);
    }
    memset(pdu,0,uiPDULen); //memset() 函数将指定的值 c 复制到 str 所指向的内存区域的前 n 个字节中  void *memset(void *str, int c, size_t n)
    pdu->uiPDULen =  uiPDULen;
    pdu->uiMsgLen =  uiMsgLen;
    return pdu;
}
