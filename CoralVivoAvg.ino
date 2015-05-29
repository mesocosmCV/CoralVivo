#include <ReefAngel_Features.h>
#include <Globals.h>
#include <RA_Wifi.h>
#include <Wire.h>
#include <OneWire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <InternalEEPROM.h>
#include <RA_NokiaLCD.h>
#include <RA_ATO.h>
#include <RA_Joystick.h>
#include <LED.h>
#include <RA_TempSensor.h>
#include <Relay.h>
#include <RA_PWM.h>
#include <Timer.h>
#include <Memory.h>
#include <InternalEEPROM.h>
#include <RA_Colors.h>
#include <RA_CustomColors.h>
#include <Salinity.h>
#include <RF.h>
#include <IO.h>
#include <ORP.h>
#include <ReefAngel.h>


#define T4TempColor         COLOR_SADDLEBROWN  // Text color for the T4 temp probe (homescreen)
#define PH1Color            COLOR_RED  // Text color for the T1 temp probe (homescreen)
#define PH2Color	        COLOR_CHOCOLATE  // Text color for the T2 temp probe (homescreen)
#define PH3Color    	    COLOR_MEDIUMORCHID  // Text color for the T3 temp probe (homescreen)
#define PH4Color        	COLOR_SADDLEBROWN  // Text color for the T4 temp probe (homescreen)

// Coral Vivo Custom pH modules
#define CheckPHCal		  708
#define Mem_I_PHMax2              710
#define Mem_I_PHMin2              712
#define Mem_I_PHMax3              714
#define Mem_I_PHMin3              716
#define Mem_I_PHMax4              718
#define Mem_I_PHMin4              720

#define HeaterDelay   180
#define LogInterval   1000
#define WifiLogInterval   300000
#define avgsize       8

int DeltaTank1=0;
int DeltaTank2=0;
int DeltaTank3=0;
int DeltaTank4=0;
int DeltaTankPH2=0;
int DeltaTankPH3=0;
int DeltaTankPH4=0;
int Hysteresis=5;
int Alert=15;
int AlertPH=30;
TimerClass HeaterTimer[5];
unsigned long lastmillisT[5];
boolean HeaterCooling[5];
unsigned long lastLog=millis();
unsigned long lastWifiLog=millis();
byte mode=0;
boolean AlertOn=false;
boolean WifiLog=false;

unsigned long avgvalue,avgvalue2,avgvalue3,avgvalue4;
int avgPH[avgsize+1];
int avgPH2[avgsize+1];
int avgPH3[avgsize+1];
int avgPH4[avgsize+1];
byte avgPtr=0;

int PHMin[5];
int PHMax[5];
int PHValue[5];

//0 - Temp only
//1 - Temp + pH
//2 - pH
//ReefAngel.Params.ORP
#include <avr/pgmspace.h>
prog_char menu1_label[] PROGMEM = "Temp mode";
prog_char menu2_label[] PROGMEM = "Temp + pH mode";
prog_char menu3_label[] PROGMEM = "pH mode";
prog_char menu4_label[] PROGMEM = "Calibrate pH 1";
prog_char menu5_label[] PROGMEM = "Calibrate pH 2";
prog_char menu6_label[] PROGMEM = "Calibrate pH 3";
prog_char menu7_label[] PROGMEM = "Calibrate pH 4";
prog_char menu8_label[] PROGMEM = "Date/Time";
prog_char menu9_label[] PROGMEM = "About Me";
PROGMEM const char *menu_items[] = {
  menu1_label, menu2_label, menu3_label, menu4_label, menu5_label, menu6_label, menu7_label, menu8_label, menu9_label};

void setup()
{
  InternalMemory.write(Mem_B_TestMode,255);
  ReefAngel.Init();  //Initialize controller
  ReefAngel.AddExtraTempProbes();
  if (InternalMemory.read_int(CheckPHCal)!=202)
  {
    InternalMemory.write_int(104,1800);
    InternalMemory.write_int(106,2900);
    InternalMemory.write_int(108,2300);
    InternalMemory.write_int(110,3400);
    InternalMemory.write_int(112,1800);
    InternalMemory.write_int(114,2800);
    InternalMemory.write_int(116,2100);
    InternalMemory.write_int(118,3100);
    InternalMemory.write_int(CheckPHCal,202);

//  Memory Settings for module with avago chip
//    InternalMemory.write_int(104,2140);
//    InternalMemory.write_int(106,3086);
//    InternalMemory.write_int(108,2140);
//    InternalMemory.write_int(110,3086);
//    InternalMemory.write_int(112,2140);
//    InternalMemory.write_int(114,3086);
//    InternalMemory.write_int(116,2140);
//    InternalMemory.write_int(118,3086);
//    InternalMemory.write_int(CheckPHCal,1984);
  }

  ReefAngel.InitMenu(pgm_read_word(&(menu_items[0])),SIZE(menu_items));
  ReefAngel.FeedingModePorts = B00000000;
  ReefAngel.WaterChangePorts = B00000000;
  ReefAngel.OverheatShutoffPorts = B00000000;
  ReefAngel.SetTemperatureUnit(1);
  for (int a=0; a<5; a++)
  {
    HeaterTimer[a].SetInterval(HeaterDelay);
    HeaterTimer[a].Start();
    HeaterTimer[a].Trigger=now();
    HeaterCooling[a]=false;
  }
  for (int a=0; a<avgsize; a++)
  {
    avgPH[a]=0;
    avgPH2[a]=0;
    avgPH3[a]=0;
    avgPH4[a]=0;
  }
  for (int a=1;a<=4;a++)
  {
    PHMin[a]=InternalMemory.read_int(100+(a*4));    
    PHMax[a]=InternalMemory.read_int(102+(a*4));   
   Serial.println( PHMin[a]);
   Serial.println( PHMax[a]);
   
  }
  lastLog=millis();
  mode=  InternalMemory.read(700);
}

void loop()
{
  if (mode==0)
  {
    DeltaTank1=265;
    DeltaTank2=20;
    DeltaTank3=20;
    DeltaTank4=20;
    DeltaTankPH2=0;
    DeltaTankPH3=0;
    DeltaTankPH4=0;
  }  
  else if (mode==1)
  {
    DeltaTank1=265;
    DeltaTank2=20;
    DeltaTank3=20;
    DeltaTank4=0;
    DeltaTankPH2=0;
    DeltaTankPH3=30;
    DeltaTankPH4=30;
  }
  else if (mode==2)
  {
    DeltaTank1=265;
    DeltaTank2=0;
    DeltaTank3=0;
    DeltaTank4=0;
    DeltaTankPH2=0;
    DeltaTankPH3=30;
    DeltaTankPH4=30;
}  
  if (mode==0)
  {
    ReefAngel.Relay.Off(Port6);
    ReefAngel.Relay.Off(Port7);
    ReefAngel.Relay.Off(Port8);
  }
  else
  {
    if(DeltaTankPH2)
    {
      if (PHValue[2]>=PHValue[1]-DeltaTankPH2+Hysteresis)
        ReefAngel.Relay.On(Port6);
      if (PHValue[2]<PHValue[1]-DeltaTankPH2-Hysteresis)
        ReefAngel.Relay.Off(Port6);
    }
    
    if(DeltaTankPH3)
    {
      if (PHValue[3]>=PHValue[1]-DeltaTankPH3+Hysteresis)
        ReefAngel.Relay.On(Port7);
      if (PHValue[3]<PHValue[1]-DeltaTankPH3-Hysteresis)
        ReefAngel.Relay.Off(Port7);
    }
    
    if(DeltaTankPH4)
    {
      if (PHValue[4]>=PHValue[1]-DeltaTankPH4+Hysteresis)
        ReefAngel.Relay.On(Port8);
      if (PHValue[4]<PHValue[1]-DeltaTankPH4-Hysteresis)
        ReefAngel.Relay.Off(Port8);
    }
    
    AlertOn=PHValue[2]>=PHValue[1]-DeltaTankPH2+Alert ||
      PHValue[2]<=PHValue[1]-DeltaTankPH2-Alert ||
      PHValue[3]>=PHValue[1]-DeltaTankPH3+Alert ||
      PHValue[3]<=PHValue[1]-DeltaTankPH3-Alert ||
      PHValue[4]>=PHValue[1]-DeltaTankPH4+Alert ||
      PHValue[4]<=PHValue[1]-DeltaTankPH4-Alert;
  }

  if (mode==2)
  {
    ReefAngel.Relay.Off(Port1);
    ReefAngel.Relay.Off(Port2);
    ReefAngel.Relay.Off(Port3);
    ReefAngel.Relay.Off(Port4);
  }
  else
  {
    if (ReefAngel.Params.Temp[1]<=DeltaTank1)
    {
      ReefAngel.Relay.On(Port1);
    }
    else
    {
      ReefAngel.Relay.Off(Port1);
      if (!HeaterCooling[1])
      {
        HeaterCooling[1]=true;
        HeaterTimer[1].Start();
        bitClear(ReefAngel.Relay.RelayMaskOff,0);
      }
    }

    if (DeltaTank2)
    {
      if (ReefAngel.Params.Temp[2]<=ReefAngel.Params.Temp[1]+DeltaTank2)
      {
        ReefAngel.Relay.On(Port2);
      }
      else
      {
        ReefAngel.Relay.Off(Port2);
        if (!HeaterCooling[2])
        {
          HeaterCooling[2]=true;
          HeaterTimer[2].Start();
          bitClear(ReefAngel.Relay.RelayMaskOff,1);
        }
      }
    }
    
    if (DeltaTank3)
    {
      if (ReefAngel.Params.Temp[3]<=ReefAngel.Params.Temp[1]+DeltaTank3)
      {
        ReefAngel.Relay.On(Port3);
      }
      else
      {
        ReefAngel.Relay.Off(Port3);
        if (!HeaterCooling[3])
        {
          HeaterCooling[3]=true;
          HeaterTimer[3].Start();
          bitClear(ReefAngel.Relay.RelayMaskOff,2);
        }
      }
    }

    if (DeltaTank4)
    {
      if (ReefAngel.Params.Temp[4]<=ReefAngel.Params.Temp[1]+DeltaTank4)
      {
        ReefAngel.Relay.On(Port4);
      }
      else
      {
        ReefAngel.Relay.Off(Port4);
        if (!HeaterCooling[4])
        {
          HeaterCooling[4]=true;
          HeaterTimer[4].Start();
          bitClear(ReefAngel.Relay.RelayMaskOff,3);
        }
      }
    }
    
    for (int a=0; a<4;a++)
    {
      if (HeaterTimer[a+1].IsTriggered())
      {
        bitSet(ReefAngel.Relay.RelayMaskOff,a);
        HeaterCooling[a+1]=false;
      }
    }

    AlertOn=ReefAngel.Params.Temp[2]>=ReefAngel.Params.Temp[1]+DeltaTank2+Alert ||
      ReefAngel.Params.Temp[2]<=ReefAngel.Params.Temp[1]+DeltaTank2-Alert ||
      ReefAngel.Params.Temp[3]>=ReefAngel.Params.Temp[1]+DeltaTank3+Alert ||
      ReefAngel.Params.Temp[3]<=ReefAngel.Params.Temp[1]+DeltaTank3-Alert ||
      ReefAngel.Params.Temp[4]>=ReefAngel.Params.Temp[1]+DeltaTank4+Alert ||
      ReefAngel.Params.Temp[4]<=ReefAngel.Params.Temp[1]+DeltaTank4-Alert;
  }

  ReefAngel.Relay.Set(5,AlertOn);

  if (millis()-lastLog>LogInterval)
  {
    
    if (avgPtr++>=avgsize) avgPtr=0;

    for (int a=1;a<=4;a++)
    {
      int temp=PHRead(a);
      if (temp>0)
        PHValue[a]=map(temp, PHMin[a], PHMax[a], 700, 1000);
      else
        PHValue[a]=0;
        
      if (PHValue[1]>750 && PHValue[1]<870)
        PHValue[1]-=7;
      if (PHValue[2]>750 && PHValue[2]<870)
        PHValue[2]-=10;
      if (PHValue[3]>750 && PHValue[3]<870)
        PHValue[3]-=10;
      if (PHValue[4]>750 && PHValue[4]<870)
        PHValue[4]-=10;
    }

    avgPH[avgPtr]=PHValue[1];
    avgPH2[avgPtr]=PHValue[2];
    avgPH3[avgPtr]=PHValue[3];
    avgPH4[avgPtr]=PHValue[4];
    
    avgvalue=0;
    avgvalue2=0;
    avgvalue3=0;
    avgvalue4=0;
    
    for (int a=0; a<avgsize; a++)
    {
      avgvalue+=avgPH[a];
      avgvalue2+=avgPH2[a];
      avgvalue3+=avgPH3[a];
      avgvalue4+=avgPH4[a];      
    }
    avgvalue/=avgsize;
    avgvalue2/=avgsize;
    avgvalue3/=avgsize;
    avgvalue4/=avgsize;
    
    PHValue[1] = avgvalue;
    PHValue[2] = avgvalue2;
    PHValue[3] = avgvalue3;
    PHValue[4] = avgvalue4;

    if (WifiLog)
    {
      if (millis()-lastWifiLog>WifiLogInterval)
      {
        Serial.print("GET /status/submitp.aspx?id=coralvivotest2&t1=");
        Serial.print(ReefAngel.Params.Temp[1]);
        Serial.print("&t2=");
        Serial.print(ReefAngel.Params.Temp[2]);
        Serial.print("&t3=");
        Serial.print(ReefAngel.Params.Temp[3]);
        Serial.print("&wl=");
        Serial.print(ReefAngel.Params.Temp[4]);
        Serial.print("&ph=");
        Serial.print(avgvalue);
        Serial.print("&phe=");
        Serial.print(avgvalue2);
        Serial.print("&sal=");
        Serial.print(avgvalue3);
        Serial.print("&orp=");
        Serial.println(avgvalue4);
        lastWifiLog=millis();
        //GET /status/submitp.aspx?t1=0&t2=0&t3=0&ph=699&id=forum_username&em=0&em1=0&rem=0&bid=0&key=&ddns=&af=2&sf=0&atohigh=0&atolow=0&r=0&ron=0&roff=255
      }
    }
    else
    {
      Serial.print(mode);
      Serial.print(",");
      Serial.print(ReefAngel.Params.Temp[1]);
      Serial.print(",");
      Serial.print(ReefAngel.Params.Temp[2]);
      Serial.print(",");
      Serial.print(ReefAngel.Params.Temp[3]);
      Serial.print(",");
      Serial.print(ReefAngel.Params.Temp[4]);
      Serial.print(",");
      Serial.print(PHValue[1]);
      Serial.print(",");
      Serial.print(PHValue[2]);
      Serial.print(",");
      Serial.print(PHValue[3]);
      Serial.print(",");
      Serial.print(PHValue[4]);
      Serial.println("");
    }
    lastLog=millis();
  }
//  ReefAngel.Relay.RelayData=255;

  ReefAngel.ShowInterface();

}

void MenuEntry1()
{
  mode=0;
  InternalMemory.write(700,0);
  ReefAngel.DisplayedMenu=RETURN_MAIN_MODE;
}

void MenuEntry2()
{
  mode=1;
  InternalMemory.write(700,1);
  ReefAngel.DisplayedMenu=RETURN_MAIN_MODE;
}

void MenuEntry3()
{
  mode=2;
  InternalMemory.write(700,2);
  ReefAngel.DisplayedMenu=RETURN_MAIN_MODE;
}

void MenuEntry4()
{
  SetupCalibratePH(1);
  ReefAngel.DisplayedMenu = ALT_SCREEN_MODE;
}

void MenuEntry5()
{
  SetupCalibratePH(2);
  ReefAngel.DisplayedMenu = ALT_SCREEN_MODE;
}

void MenuEntry6()
{
  SetupCalibratePH(3);
  ReefAngel.DisplayedMenu = ALT_SCREEN_MODE;
}

void MenuEntry7()
{
  SetupCalibratePH(4);
  ReefAngel.DisplayedMenu = ALT_SCREEN_MODE;
}

void MenuEntry8()
{
}

void MenuEntry9()
{
  ReefAngel.DisplayVersion();
}

void DrawCustomMain()
{
  // the graph is drawn/updated when we exit the main menu &
  // when the parameters are saved
  ReefAngel.LCD.Clear(BtnActiveColor,5,0,127,11);
  ReefAngel.LCD.DrawText(DefaultBGColor,BtnActiveColor,40,3,"Coral Vivo");
  ReefAngel.LCD.DrawDate(6, 122);

  int x=8;
  int y=20;
  char text[10];
  if (mode<2)
  {
    ReefAngel.LCD.DrawText(T1TempColor,DefaultBGColor,x,y,"T1:");
    ConvertNumToString(text, ReefAngel.Params.Temp[1], 10);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(T1TempColor, DefaultBGColor, x+23, y,text);
    signed long t=HeaterTimer[1].Trigger-now();
    if (t<0) t=0;
    ConvertNumToString(text, t, 1);
    strcat(text,"      ");
    ReefAngel.LCD.DrawText(T1TempColor, DefaultBGColor, x, y+11,text);

    ReefAngel.LCD.DrawText(T2TempColor,DefaultBGColor,x+60,y,"T2:");
    ConvertNumToString(text, ReefAngel.Params.Temp[2], 10);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(T2TempColor, DefaultBGColor, x+83, y,text);
    t=HeaterTimer[2].Trigger-now();
    if (t<0) t=0;
    ConvertNumToString(text, t, 1);
    strcat(text,"      ");
    ReefAngel.LCD.DrawText(T2TempColor, DefaultBGColor, x+83, y+11,text);

    y+=20;

    ReefAngel.LCD.DrawText(T3TempColor,DefaultBGColor,x,y,"T3:");
    ConvertNumToString(text, ReefAngel.Params.Temp[3], 10);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(T3TempColor, DefaultBGColor, x+23, y,text);
    t=HeaterTimer[3].Trigger-now();
    if (t<0) t=0;
    ConvertNumToString(text, t, 1);
    strcat(text,"      ");
    ReefAngel.LCD.DrawText(T3TempColor, DefaultBGColor, x, y+11,text);

    ReefAngel.LCD.DrawText(T4TempColor,DefaultBGColor,x+60,y,"T4:");
    ConvertNumToString(text, ReefAngel.Params.Temp[4], 10);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(T4TempColor, DefaultBGColor, x+83, y,text);
    t=HeaterTimer[4].Trigger-now();
    if (t<0) t=0;
    ConvertNumToString(text, t, 1);
    strcat(text,"      ");
    ReefAngel.LCD.DrawText(T4TempColor, DefaultBGColor, x+83, y+11,text);
  }
  y+=20;
  if (mode!=0)
  {
    ReefAngel.LCD.DrawText(PH1Color,DefaultBGColor,x,y,"P1:");
    ConvertNumToString(text, PHValue[1], 100);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(PH1Color, DefaultBGColor, x+23, y,text);

    ReefAngel.LCD.DrawText(PH2Color,DefaultBGColor,x+60,y,"P2:");
    ConvertNumToString(text, PHValue[2], 100);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(PH2Color, DefaultBGColor, x+83, y,text);

    y+=20;

    ReefAngel.LCD.DrawText(PH3Color,DefaultBGColor,x,y,"P3:");
    ConvertNumToString(text, PHValue[3], 100);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(PH3Color, DefaultBGColor, x+23, y,text);

    ReefAngel.LCD.DrawText(PH4Color,DefaultBGColor,x+60,y,"P4:");
    ConvertNumToString(text, PHValue[4], 100);
    strcat(text,"  ");
    ReefAngel.LCD.DrawText(PH4Color, DefaultBGColor, x+83, y,text);
  }



  byte TempRelay = ReefAngel.Relay.RelayData;
  TempRelay &= ReefAngel.Relay.RelayMaskOff;
  TempRelay |= ReefAngel.Relay.RelayMaskOn;
  ReefAngel.LCD.DrawOutletBox(12, 103, TempRelay);
}

void DrawCustomGraph()
{
  //  ReefAngel.LCD.DrawGraph(5, 15);
}

int PHRead(int channel)
{
  unsigned long tempPH=0;
  
//  for (int a=0;a<10;a++)
//  {
    int iPH=0;
    
    switch (channel)
    {
    case 1:
      Wire.requestFrom(0x4d, 2);
      break;
    case 2:
      Wire.requestFrom(0x48, 2);
      break;
    case 3:
      Wire.requestFrom(0x4e, 2);
      break;
    case 4:
      Wire.requestFrom(0x4f, 2);
      break;
    }  
    if (Wire.available())
    {
      iPH = Wire.read();
      iPH = iPH<<8;
      iPH += Wire.read();
    }
    return iPH;
//    tempPH+=iPH;
//  }
//  tempPH/=10;
//  return tempPH;
}

void SetupCalibratePH(int channel)
{
  bool bOKSel = false;
  bool bSave = false;
  bool bDone = false;
  bool bDrawButtons = true;
  unsigned int iO[2] = {
    0,0      };
  unsigned int iCal[2] = {
    7,10      };
  byte offset = 65;
  // draw labels
  ReefAngel.ClearScreen(DefaultBGColor);
  for (int b=0;b<2;b++)
  {
    if (b==1 && !bSave) break;
    bOKSel=false;
    bSave=false;
    bDone = false;
    bDrawButtons = true;
    ReefAngel.LCD.DrawText(DefaultFGColor, DefaultBGColor, MENU_START_COL, MENU_START_ROW, "Calibrate pH");
    ReefAngel.LCD.DrawText(DefaultFGColor, DefaultBGColor, MENU_START_COL + 72, MENU_START_ROW, channel);
    ReefAngel.LCD.DrawText(DefaultFGColor, DefaultBGColor, MENU_START_COL, MENU_START_ROW*5, "pH");
    ReefAngel.LCD.DrawText(DefaultFGColor, DefaultBGColor, MENU_START_COL + 18, MENU_START_ROW*5, (int)iCal[b]);
    do
    {
#if defined WDT || defined WDT_FORCE
      wdt_reset();
#endif  // defined WDT || defined WDT_FORCE
      iO[b]=0;
      for (int a=0;a<10;a++)
      {
        iO[b] += PHRead(channel);
        wdt_reset();
        delay(10);
      }
      iO[b]/=10;
//      Serial.println(iO[b]);
//      delay(1000);
      ReefAngel.LCD.DrawCalibrate(iO[b], MENU_START_COL + offset, MENU_START_ROW*5);
      if (  bDrawButtons )
      {
        if ( bOKSel )
        {
          ReefAngel.LCD.DrawOK(true);
          ReefAngel.LCD.DrawCancel(false);
        }
        else
        {
          ReefAngel.LCD.DrawOK(false);
          ReefAngel.LCD.DrawCancel(true);
        }
        bDrawButtons = false;
      }
      if ( ReefAngel.Joystick.IsUp() || ReefAngel.Joystick.IsDown() || ReefAngel.Joystick.IsRight() || ReefAngel.Joystick.IsLeft() )
      {
        // toggle the selection
        bOKSel = !bOKSel;
        bDrawButtons = true;
      }
      if ( ReefAngel.Joystick.IsButtonPressed() )
      {
        bDone = true;
        if ( bOKSel )
        {
          bSave = true;
        }
      }
    } 
    while ( ! bDone );
  }
  ReefAngel.ClearScreen(DefaultBGColor);
  if ( bSave )
  {
    // save PHMin & PHMax to memory
    InternalMemory.write_int(100+(channel*4),iO[0]);
    PHMin[channel] = iO[0];
    InternalMemory.write_int(102+(channel*4),iO[1]);
    PHMax[channel] = iO[1];
  }
}

