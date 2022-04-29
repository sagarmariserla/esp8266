#define esp32
#include"common.h"
         
void setup() 
{
  Serial.begin(BAUDRATE);
  EEPROM.begin(EEPROM_MEM_SIZE);
  Serial.print("Hardware firmware version : ");
  Serial.println(VERSION);
  #ifdef esp8266
  sprintf(uniqueid, "%08X",ESP.getChipId());
  #else
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  sprintf(uniqueid, "%08X",chipId);
  Serial.print("uniqueid : ");
  Serial.println(uniqueid);
  #endif
  pins_config();
  switch_initialization();
    
  //UDP.begin(UDP_RCV_PORT);
  Serial.print("Listening on UDP port... ");
  Serial.println(UDP_RCV_PORT);
  
  get_reg_status();
  get_locationid_eeprom();
  get_userid_eeprom();
  
  get_dev_status_from_FS();
  if(reg_status != REG_DONE) 
  {
    Create_config_file();
  }
  else 
  {
    network_reconnect();
  }  
}


void loop() 
{
  RX_BUF rx_buf;

  if((!client_cloudmqtt.connected()) && WiFi.status() == WL_CONNECTED && (reg_status == REG_DONE))   //mqtt client connection verification for cloud
  {
    reconnect_cloudmqtt();  
  }
  
  reg_pattern.read();                                                                //This function is used to detect long press of button
  dereg_pattern.read(); 
  firmware_update_button.read();
  
  if(reg_status == REG_DONE) 
  {
      udp_packet_receiver(udp_packet);                                                    //Function is used to read the packets from udp
      if(strlen(udp_packet) != EMPTY) 
      {
        parse_request(udp_packet, &rx_buf);
        process_request(&rx_buf);
        memset(udp_packet, '\0', strlen(udp_packet));
      }
  }

  if(reg_status != REG_DONE && credentials_recv == SUCCESSFUL) 
  {       
    delay(BASIC_DELAY);   
    process_request(&registration_data_buf);
    credentials_recv = EMPTY;
    memset(&registration_data_buf, '\0', sizeof(RX_BUF));
    device_under_registration = false; 
  }
  
  if(reg_status == REG_DONE) 
  {
    client_cloudmqtt.loop();
  }

  if (switch_intrp_inprog == true)
  {
    if(WiFi.status()== WL_CONNECTED && (reg_status == REG_DONE) && (trigger_type == false)&& (client_cloudmqtt.connected()))
    {
      device_status_update(device_stat_update, endpoint_num, MANUAL_CONTROL);                                 // Device status update when manual button is triggered
    }
    else if(WiFi.status()== WL_CONNECTED && (reg_status == REG_DONE) && (trigger_type == true) && (client_cloudmqtt.connected()))
    {
      device_status_update(device_stat_update, endpoint_num, MOBILE_CONTROL);                                 // Device status update when manual button is triggered
    }
    
    fs_save_stat(endpoint_num, device_stat_update);
    switch_intrp_inprog = false;
  }

  if (device_under_registration == true)
  {
    connect_wifi();
  }

  if (device_under_deregistration == true)
  {
    if (mobile_dereg == false)
    { 
      http_exclusion_api_call(MANUAL_DEREG);     
    }
    if (mobile_dereg == true)
    { 
      http_exclusion_api_call(MOBILE_DEREG);
      mobile_dereg = false;
    }
    device_under_deregistration = false;
    device_under_registration = true;   
  }
  if((fota_update==true) && (reg_status == REG_DONE))
  {
    firmware_update();
  }
  delay(50);
}
