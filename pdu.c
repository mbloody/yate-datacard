/*
SMS Server Tools 3
Copyright (C) Keijo Kasvi
http://smstools3.kekekasvi.com/

Based on SMS Server Tools 2 from Stefan Frings
http://www.meinemullemaus.de/

This program is free software unless you got it under another license directly
from the author. You can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation.
Either version 2 of the License, or (at your option) any later version.
*/

#include "pdu.h"
#include "smsd_cfg.h"
#include "string.h"
#include "stdio.h"

/* Swap every second character */
void swapchars(char* string) 
{
  int Length;
  int position;
  char c;
  Length=strlen(string);
  for (position=0; position<Length-1; position+=2)
  {
    c=string[position];
    string[position]=string[position+1];
    string[position+1]=c;
  }
}

// Converts an ascii text to a pdu string 
// text might contain zero values because this is a valid character code in sms
// character set. Therefore we need the length parameter.
// If udh is not 0, then insert it to the beginning of the message.
// The string must be in hex-dump format: "05 00 03 AF 02 01". 
// The first byte is the size of the UDH.
int text2pdu(char* text, int length, char* pdu, char* udh)
{
  char tmp[500];
  char octett[10];
  int pdubitposition;
  int pdubyteposition=0;
  int character;
  int bit;
  int pdubitnr;
  int counted_characters=0;
  int udh_size_octets;   // size of the udh in octets, should equal to the first byte + 1
  int udh_size_septets;  // size of udh in septets (7bit text characters)
  int fillbits;          // number of filler bits between udh and ud.
  int counter;
#ifdef DEBUGMSG
  printf("!! text2pdu(text=..., length=%i, pdu=..., udh=%s\n",length,udh);
#endif
  pdu[0]=0;
  // Check the udh
  if (udh)
  {
    udh_size_octets=(strlen(udh)+2)/3;
    udh_size_septets=((udh_size_octets)*8+6)/7;
    fillbits=7-(udh_size_octets % 7);
    if (fillbits==7)
      fillbits=0;

    // copy udh to pdu, skipping the space characters
    for (counter=0; counter<udh_size_octets; counter++)
    {
      pdu[counter*2]=udh[counter*3];
      pdu[counter*2+1]=udh[counter*3+1];
    }
    pdu[counter*2]=0;
#ifdef DEBUGMSG
  printf("!! pdu=%s, fillbits=%i\n",pdu,fillbits);
#endif
  } 
  else
  {
    udh_size_octets=0;
    udh_size_septets=0; 
    fillbits=0;
  }
  // limit size of the message to maximum allowed size
  if (length>maxsms_pdu-udh_size_septets)
    length=maxsms_pdu-udh_size_septets;
  //clear the tmp buffer
  for (character=0;character<sizeof(tmp);character++)
    tmp[character]=0;
  // Convert 8bit text stream to 7bit stream
  for (character=0;character<length;character++)
  {
     counted_characters++;
    for (bit=0;bit<7;bit++)
    {
      pdubitnr=7*character+bit+fillbits;
      pdubyteposition=pdubitnr/8;
      pdubitposition=pdubitnr%8;
      if (text[character] & (1<<bit))
        tmp[pdubyteposition]=tmp[pdubyteposition] | (1<<pdubitposition);
      else
        tmp[pdubyteposition]=tmp[pdubyteposition] & ~(1<<pdubitposition);
    }
  }
  tmp[pdubyteposition+1]=0;
  // convert 7bit stream to hex-dump
  for (character=0;character<=pdubyteposition; character++)
  {
    sprintf(octett,"%02X",(unsigned char) tmp[character]);
    strcat(pdu,octett);
  }
#ifdef DEBUGMSG
  printf("!! pdu=%s\n",pdu);
#endif
  return counted_characters+udh_size_septets;
}

/* Converts binary to PDU string, this is basically a hex dump. */
void binary2pdu(char* binary, int length, char* pdu)
{
  int character;
  char octett[10];
  if (length>maxsms_binary)
    length=maxsms_binary;
  pdu[0]=0;
  for (character=0;character<length; character++)
  {
    sprintf(octett,"%02X",(unsigned char) binary[character]);
    strcat(pdu,octett);
  }
}

// Make the PDU string from a mesage text and destination phone number.
// The destination variable pdu has to be big enough. 
// alphabet indicates the character set of the message.
// flash_sms enables the flash flag.
// mode select the pdu version (old or new).
// if udh is true, then udh_data contains the optional user data header in hex dump, example: "05 00 03 AF 02 01"

void make_pdu(char* number, char* message, int messagelen, int alphabet, int flash_sms, int report, int with_udh, char* udh_data, char* mode, char* pdu, int validity)
{
  int coding;
  int flags;
  char tmp[50];
  char tmp2[500];
  int numberformat;
  int numberlength;

  if (number[0]=='s')  // Is number starts with s, then send it without number format indicator
  {
    numberformat=129;
    strcpy(tmp,number+1);
  }
  else
  {
    numberformat=145;
    strcpy(tmp,number);
  }
  numberlength=strlen(tmp);
  // terminate the number with F if the length is odd
  if (numberlength%2)
    strcat(tmp,"F");
  // Swap every second character
  swapchars(tmp);

  flags=1; // SMS-Sumbit MS to SMSC
  if (with_udh)
    flags+=64; // User Data Header
  if (strcmp(mode,"old")!=0)
    flags+=16; // Validity field
  if (report>0)
    flags+=32; // Request Status Report

  if (alphabet == 1)
    coding = 4; // 8bit binary
  else if (alphabet == 2)
    coding = 8; // 16bit
  else
    coding = 0; // 7bit
  if (flash_sms > 0)
    coding += 0x10; // Bits 1 and 0 have a message class meaning (class 0, alert)

  /* Create the PDU string of the message */
  if (alphabet==1 || alphabet==2)
    binary2pdu(message,messagelen,tmp2);
  else
    messagelen=text2pdu(message,messagelen,tmp2,udh_data);
  /* concatenate the first part of the PDU string */
  if (strcmp(mode,"old")==0)
    sprintf(pdu,"%02X00%02X%02X%s00%02X%02X",flags,numberlength,numberformat,tmp,coding,messagelen);
  else
  {
    if (validity < 0 || validity > 255)
      validity = validity_period;
    sprintf(pdu,"00%02X00%02X%02X%s00%02X%02X%02X",flags,numberlength,numberformat,tmp,coding,validity,messagelen);
  }
  /* concatenate the text to the PDU string */
  strcat(pdu,tmp2);
}


int octet2bin(char* octet) /* converts an octet to a 8-Bit value */
{
  int result=0;
  if (octet[0]>57)
    result+=octet[0]-55;
  else
    result+=octet[0]-48;
  result=result<<4;
  if (octet[1]>57)
    result+=octet[1]-55;
  else
    result+=octet[1]-48;
  return result;
}


int pdu2text(char* pdu, char* text, int with_udh, char* udh) 
                                                     /* converts a PDU-String to text, text might contain zero values! */
{                                                    /* the first octet is the length */
  int bitposition=0;                                 /* return the length of text */
  int byteposition;                                  /* with_udh must be set already if the message has an UDH */
  int byteoffset;                                    /* this function does not detect the existance of UDH automatically. */
  int charcounter;
  int bitcounter;
  int septets;
  int octets;
  int udhsize;
  int octetcounter;
  int skip_characters;
  char c;
  char binary[500];
#ifdef DEBUGMSG
  printf("!! pdu2text(pdu=%s,...)\n",pdu);
#endif

  septets=octet2bin(pdu);

  if (with_udh)
  {
    // copy the data header to udh and convert to hex dump
    udhsize=octet2bin(pdu+2);    
    for (octetcounter=0; octetcounter<udhsize+1; octetcounter++)
    {
      udh[octetcounter*3]=pdu[(octetcounter<<1)+2];
      udh[octetcounter*3+1]=pdu[(octetcounter<<1)+3];
      udh[octetcounter*3+2]=' '; 
    }
    // append null termination to udh string.
    udh[octetcounter*3]=0;
    // Calculate how many text charcters include the UDH.
    // After the UDH there may follow filling bits to reach a 7bit boundary.
    skip_characters=(((udhsize+1)*8)+6)/7;
#ifdef DEBUGMSG
  printf("!! septets=%i\n",septets);
  printf("!! udhsize=%i\n",udhsize);
  printf("!! skip_characters=%i\n",skip_characters);
#endif
  }
  else
  {
    if (udh) 
      udh[0]=0;
    skip_characters=0;
  }

  // Copy user data of pdu to a buffer
  octets=(septets*7+7)/8;     
  for (octetcounter=0; octetcounter<octets; octetcounter++)
    binary[octetcounter]=octet2bin(pdu+(octetcounter<<1)+2);

  // Then convert from 8-Bit to 7-Bit encapsulated in 8 bit 
  // skipping storing of some characters used by UDH.
  for (charcounter=0; charcounter<septets; charcounter++)
  {
    c=0;
    for (bitcounter=0; bitcounter<7; bitcounter++)
    {
      byteposition=bitposition/8;
      byteoffset=bitposition%8;
      if (binary[byteposition]&(1<<byteoffset))
        c=c|128;
      bitposition++;
      c=(c>>1)&127; /* The shift fills with 1, but I want 0 */
    }
    if (charcounter>=skip_characters)
      text[charcounter-skip_characters]=c; 
  }
  text[charcounter-skip_characters]=0;
  return charcounter-skip_characters;
}

/* Converts a PDU string to binary */
int pdu2binary(char* pdu, char* binary)
{
  int octets;
  int octetcounter;
  octets=octet2bin(pdu);
  for (octetcounter=0; octetcounter<octets; octetcounter++)
    binary[octetcounter]=octet2bin(pdu+(octetcounter<<1)+2);
  binary[octets]=0;
  return octets;
}



// Subroutine for splitpdu() for messages type 0 (SMS-Deliver)
// Input:
// Src_Pointer points to the PDU string
// Output:
// sendr Sender
// date and time Date/Time-stamp
// message the message text or binary data
// returns length of message
int split_type_0(char* Src_Pointer, int* alphabet, char* sendr, char* date, char* time, char* message, int with_udh, char* udh)
{
  int Length;
  int padding;
  char tmpsender[100];
#ifdef DEBUGMSG
  printf("!! split_type_0(Src_Pointer=%s, ...\n",Src_Pointer);
#endif
  Length=octet2bin(Src_Pointer);
  padding=Length%2;
  Src_Pointer+=2;
  if ((octet2bin(Src_Pointer) & 112)==80)
  {  // Sender is alphanumeric
    Src_Pointer+=2;
    if (Length>sizeof(tmpsender)-3-padding)  //Limit length of senders name
      Length=sizeof(tmpsender)-3-padding;
    snprintf(tmpsender,Length+3+padding,"%02x%s",Length*4/7,Src_Pointer);
    pdu2text(tmpsender,sendr,0,0);
  }
  else
  {  // sender is numeric
    Src_Pointer+=2;
    strncpy(sendr,Src_Pointer,Length+padding);
    swapchars(sendr);
    sendr[Length]=0;
    if (sendr[Length-1]=='F')
      sendr[Length-1]=0;
  }
  Src_Pointer=Src_Pointer+Length+padding+3;
  *alphabet=(Src_Pointer[0] & 12) >>2;
  if (*alphabet==0)
    *alphabet=-1;
  Src_Pointer++;
  sprintf(date,"%c%c-%c%c-%c%c",Src_Pointer[1],Src_Pointer[0],Src_Pointer[3],Src_Pointer[2],Src_Pointer[5],Src_Pointer[4]);
  Src_Pointer=Src_Pointer+6;
  sprintf(time,"%c%c:%c%c:%c%c",Src_Pointer[1],Src_Pointer[0],Src_Pointer[3],Src_Pointer[2],Src_Pointer[5],Src_Pointer[4]);
  Src_Pointer=Src_Pointer+8;

  // Version 2.x way:
  //if (*alphabet<1)
  //  return pdu2text(Src_Pointer,message,with_udh,udh);
  //else  
  //  return pdu2binary(Src_Pointer,message);

  // Version 3.x way:
  if (*alphabet<1)
    return pdu2text(Src_Pointer,message,with_udh,udh);
  else if (*alphabet != 2 || with_udh == 0)
    return pdu2binary(Src_Pointer,message);
  else
  {
    // Alphabet:UCS2(16)bit
    // If it seems that the message is a part of a concatenated message, header bytes are removed from
    // the begin of the message and values of these are printed to the udh buffer.
    // UDH-DATA: "05 00 03 02 03 02 "
    // NOTE: If the whole message consist of single part only, there still can be (or always is?) a
    // concatenated header with values partcount=1 and partnr=1.
    int octets;
    octets = pdu2binary(Src_Pointer,message);
    if (octets > 6)
      if (message[0] == 0x05 && message[1] == 0x00 && message[2] == 0x03 &&
          message[4] > 0x00 &&
          message[5] > 0x00 &&
          message[5] <= message[4])
      {
        sprintf(udh, "%02X %02X %02X %02X %02X %02X ", message[0], message[1], message[2], message[3], message[4], message[5]);
        octets -= 6;
        memcpy(message, message +6, octets);
      }

    return octets;
  }

}


// Subroutine for splitpdu() for messages type 2 (Status Report)
// Input: 
// Src_Pointer points to the PDU string
// Output:
// sendr Sender
// date and time Date/Time-stamp
// result is the status value and text translation

int split_type_2(char* Src_Pointer,char* sendr, char* date,char* time,char* result)
{
  int length;
  int padding;
  int status;
  char temp[32];
  char tmpsendr[100];
  int messageid;
  strcat(result,"SMS STATUS REPORT\n");
  // get message id
  messageid=octet2bin(Src_Pointer);
  sprintf(temp,"Message_id: %i\n",messageid);
  strcat(result,temp);
  // get recipient address
  Src_Pointer+=2;
  length=octet2bin(Src_Pointer);
  padding=length%2;
  Src_Pointer+=2;
  if ((octet2bin(Src_Pointer) & 112)==80)
  {  // Sender is alphanumeric
    Src_Pointer+=2;
    sprintf(tmpsendr,"%02x%s",length*4/7,Src_Pointer);
    pdu2text(tmpsendr,sendr,0,0);
  }
  else
  {  // sender is numeric
    Src_Pointer+=2;
    strncpy(sendr,Src_Pointer,length+padding);
    swapchars(sendr);
    sendr[length]=0;
    if (sendr[length-1]=='F')
      sendr[length-1]=0;
  }
  // get SMSC timestamp
  Src_Pointer+=length+padding;
  sprintf(date,"%c%c-%c%c-%c%c",Src_Pointer[1],Src_Pointer[0],Src_Pointer[3],Src_Pointer[2],Src_Pointer[5],Src_Pointer[4]);
  sprintf(time,"%c%c:%c%c:%c%c",Src_Pointer[7],Src_Pointer[6],Src_Pointer[9],Src_Pointer[8],Src_Pointer[11],Src_Pointer[10]);
  // get Discharge timestamp
  strcat(result,"Discharge_timestamp: ");
  Src_Pointer+=14;
  sprintf(temp,"%c%c-%c%c-%c%c %c%c:%c%c:%c%c",Src_Pointer[1],Src_Pointer[0],Src_Pointer[3],Src_Pointer[2],Src_Pointer[5],Src_Pointer[4],Src_Pointer[7],Src_Pointer[6],Src_Pointer[9],Src_Pointer[8],Src_Pointer[11],Src_Pointer[10]);
  strcat(result,temp);
  // get Status
  strcat(result,"\nStatus: ");
  Src_Pointer+=14;
  status=octet2bin(Src_Pointer);
  sprintf(temp,"%i,",status);
  strcat(result,temp);
  switch (status)
  {
    case 0: strcat(result,"Ok,short message received by the SME"); break;
    case 1: strcat(result,"Ok,short message forwarded by the SC to the SME but the SC is unable to confirm delivery"); break;
    case 2: strcat(result,"Ok,short message replaced by the SC"); break;

    case 32: strcat(result,"Still trying,congestion"); break;
    case 33: strcat(result,"Still trying,SME busy"); break;
    case 34: strcat(result,"Still trying,no response sendr SME"); break;
    case 35: strcat(result,"Still trying,service rejected"); break;
    case 36: strcat(result,"Still trying,quality of service not available"); break;
    case 37: strcat(result,"Still trying,error in SME"); break;

    case 64: strcat(result,"Error,remote procedure error"); break;
    case 65: strcat(result,"Error,incompatible destination"); break;
    case 66: strcat(result,"Error,connection rejected by SME"); break;
    case 67: strcat(result,"Error,not obtainable"); break;
    case 68: strcat(result,"Error,quality of service not available"); break;
    case 69: strcat(result,"Error,no interworking available"); break;
    case 70: strcat(result,"Error,SM validity period expired"); break;
    case 71: strcat(result,"Error,SM deleted by originating SME"); break;
    case 72: strcat(result,"Error,SM deleted by SC administration"); break;
    case 73: strcat(result,"Error,SM does not exist"); break;

    case 96: strcat(result,"Error,congestion"); break;
    case 97: strcat(result,"Error,SME busy"); break;
    case 98: strcat(result,"Error,no response sendr SME"); break;
    case 99: strcat(result,"Error,service rejected"); break;
    case 100: strcat(result,"Error,quality of service not available"); break;
    case 101: strcat(result,"Error,error in SME"); break;

    default: strcat(result,"unknown");
  }
  return strlen(result);
}



// Splits a PDU string into the parts 
// Input: 
// pdu is the pdu string
// mode can be old or new and selects the pdu version
// Output:
// alphabet indicates the character set of the message.
// sendr Sender
// date and time Date/Time-stamp
// message is the message text or binary message
// smsc that sent this message
// with_udh return the udh flag of the message
// is_statusreport is 1 if this was a status report
// is_unsupported_pdu is 1 if this pdu was not supported
// Returns the length of the message 

int splitpdu(char* pdu, char* mode, int* alphabet, char* sendr, char* date, char* time, char* message, char* smsc, int* with_udh, char* udh_data, int* is_statusreport, int* is_unsupported_pdu)
{
  int Length;
  int Type;
  char* Pointer;
  sendr[0]=0;
  date[0]=0;
  time[0]=0;
  message[0]=0;
  smsc[0]=0; 
  *alphabet=0;
  *with_udh=0;
  *is_statusreport=0;
  *is_unsupported_pdu=0;
  *message=0;  
#ifdef DEBUGMSG
  printf("!! splitpdu(pdu=%s, mode=%s, ...)\n",pdu,mode);
#endif

  Pointer=pdu;
  if (strcmp(mode,"old")!=0)
  {
    // get senders smsc
    Length=octet2bin(Pointer)*2-2;
    if (Length>0)
    {
      Pointer=Pointer+4;
      strncpy(smsc,Pointer,Length);
      swapchars(smsc);
      smsc[Length]=0;
      if (smsc[Length-1]=='F')
        smsc[Length-1]=0;
    }
    Pointer=Pointer+Length;
  }
  if (octet2bin(Pointer) & 64) // Is UDH bit set?
    *with_udh=1;
  Type=octet2bin(Pointer) & 3;
  Pointer+=2;
  if (Type==0) // SMS Deliver
    return split_type_0(Pointer,alphabet,sendr,date,time,message,*with_udh,udh_data);
  else if (Type==2)  // Status Report
  {
    *is_statusreport=1;
    return split_type_2(Pointer,sendr,date,time,message);
  }
  else if (Type==1) // Sent message
  {
    *is_unsupported_pdu = 1;
    sprintf(message,"This is a sent message, can only decode received messages.\n");
    return strlen(message);
  }  
  else
  // Unsupported type
  {
    *is_unsupported_pdu = 1;
    sprintf(message,"Message format (%i) is not supported. Cannote decode.\n",Type);
    return strlen(message);
  }
}

