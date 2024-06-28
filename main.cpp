#include "mbed.h"
#include "HEPTA_CDH.h"
#include "HEPTA_EPS.h"
#include "HEPTA_SENSOR.h"
#include "HEPTA_COM.h"
HEPTA_CDH cdh(p5, p6, p7, p8, "sd");
HEPTA_EPS eps(p16,p26);
HEPTA_SENSOR sensor(p17,
                  p28,p27,0x19,0x69,0x13,
                  p13, p14,p25,p24);
HEPTA_COM com(p9,p10,9600);
DigitalOut condition(LED1);
RawSerial sat(USBTX,USBRX,9600);
Timer sattime;
int rcmd = 0,cmdflag = 0; //command variable

int main() {
    sat.printf("From Sat : Nominal Operation\r\n");
    com.printf("From Sat : Nominal Operation\r\n");

    float batvol, temp;                     //voltage, temperature 
    int flag = 0,rcmd=0,cmdflag=0;          //command variable
    sattime.start();
    eps.turn_on_regulator();                //turn on 3.3V conveter
    for(int i = 0; i < 100; i++) {
        com.xbee_receive(&rcmd,&cmdflag);   //interupting by ground station command
        condition = !condition;             //satellite condition led
        eps.vol(&batvol);                   //senssing battery voltage
        sensor.temp_sense(&temp);           //senssing temperature

        //Transmitting HK data to Ground Station(GS)
        com.printf("HEPTASAT::Condition = %d, Time = %f [s], batVol = %.2f [V],Temp = %.2f [C]\r\n",flag,sattime.read(),batvol,temp);
        wait_ms(1000);
                
        //Power Saving Mode 
        if((batvol <= 3.5)  | (temp > 35.0)){
            eps.shut_down_regulator();
            com.printf("Power saving mode ON\r\n"); 
            flag = 1;
        } else if((flag == 1) & (batvol > 3.7) & (temp <= 25.0)) {
            eps.turn_on_regulator();
            com.printf("Power saving mode OFF\r\n");
            flag = 0;
        }
        //Contents of command
        if (cmdflag == 1) {
            if (rcmd == 'a') {          //send hello world
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("send hello world\r\n");
                for(int j=0;j<5;j++){
                    com.printf("Hello World!\r\n");
                    condition = 1;
                    wait_ms(1000);
                }
            }else if (rcmd == 'b') {    //voltage to memory
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("write battery voltage to SD card\r\n");
                char str[100];
                mkdir("/sd/mydir", 0777);
                FILE *fp = fopen("/sd/mydir/satdata.txt","w");
                if(fp == NULL) {
                    error("Could not open file for write\r\n");
                }
                for(int i = 0; i < 10; i++) {
                    eps.vol(&batvol);
                    fprintf(fp, "Time = %.2f [s] Vol = %.2f [V]\r\n", sattime.read(),batvol);
                    wait(0.1);  
                    condition = 1;
                    wait_ms(1000);
                }
                fclose(fp);
                fp = fopen("/sd/mydir/satdata.txt","r");
                for(int i = 0; i < 10; i++) {
                    fgets(str,100,fp);
                    com.puts(str);
                }
                fclose(fp);     
                com.printf("finish!\r\n");           
            }else if (rcmd == 'c') {      //measuring temperature
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("measuring temperature\r\n");
                float temp;
                for (int i=0;i<10;i++) {
                    sensor.temp_sense(&temp);
                    com.printf("Time = %.2f [s] temp = %f [deg]\r\n",sattime.read(),temp);
                    wait(1.0);
                }
            }else if (rcmd == 'd') {      //senssing 9axis
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("senssing 9axis\r\n");
                float ax,ay,az; 
                float gx,gy,gz;
                float mx,my,mz;
                for(int i = 0; i<10; i++) {
                    sensor.sen_acc(&ax,&ay,&az);
                    sensor.sen_gyro(&gx,&gy,&gz);
                    sensor.sen_mag(&mx,&my,&mz);
                    com.printf(" Time = %.2f [s]",sattime.read());
                    com.printf(" acc : %f,%f,%f",ax,ay,az);
                    com.printf(" gyro: %f,%f,%f",gx,gy,gz);
                    com.printf(" mag : %f,%f,%f\r\n",mx,my,mz);
                    wait(0.5);
                }
            }else if (rcmd == 'e') {      //camera snap shot
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("camera snap shot\r\n");
                FILE *dummy = fopen("/sd/dummy.txt","w");
                if(dummy == NULL) {
                    error("Could not open file for write\r\n");
                }
                fclose(dummy);
                com.printf("Camera Snapshot Mode\r\n");
                sensor.Sync();
                sensor.initialize(HeptaCamera_GPS::Baud115200, HeptaCamera_GPS::JpegResolution320x240);
                sensor.test_jpeg_snapshot_picture("/sd/test.jpg");
                com.printf("Photo successfully saved to the SD card\r\n");
            }else if (rcmd == 'f') {      //gps
                sat.printf("rcmd=%c,cmdflag=%d\r\n",rcmd,cmdflag);
                com.printf("Hepta-Sat Lite Uplink Ok\r\n");
                com.printf("get gps\r\n");
                sensor.gps_setting();
                com.printf("GPS Raw Data Mode\r\n");
                for (int i=0;i<1000;i++) {com.putc(sensor.getc());}
            }
            com.initialize();
        }
    }
    sattime.stop();
    sat.printf("From Sat : End of operation\r\n");
    com.printf("From Sat : End of operation\r\n");
}
