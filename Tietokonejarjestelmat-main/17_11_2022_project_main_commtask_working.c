#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include "Board.h"
#include "sensors/mpu9250.h"
#include <math.h>
#include <wireless/comm_lib.h>
#include <wireless/address.h>
#define STACKSIZE 2048
Char taskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];
// MPU power pin global variables
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;
static PIN_Handle greenLedHandle0;
static PIN_State ledState0;
static PIN_Handle redLedHandle1;
static PIN_State ledState1;
static PIN_Handle;


// MPU power pin
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
PIN_Config ledConfig0[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};
PIN_Config ledConfig1[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};
PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina tällä vakiolla
};

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

float sensor_data[7][100];
int feed;
int exercise;
int pet;
char feeding[16];
char exercising[16];
char petting[16];

Void sensorFxn(UArg arg0, UArg arg1) {


    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    // Note the different configuration below
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 100ms for the MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }

    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();

    mpu9250_setup(&i2cMPU);

    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();

    while(1){

        float ax, ay, az, gx, gy, gz;
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

        printf("%.2f, %.2f, %.2f\n", ax,ay,az);
        if(fabs(ax) > 0.3 && fabs(ax) < 1.5 && fabs(ay) < 0.5) {
            uint_t pinValue = PIN_getOutputValue( Board_LED0 );
            pinValue = !pinValue;
            PIN_setOutputValue( greenLedHandle0, Board_LED0, pinValue );
            Task_sleep(200000/Clock_tickPeriod);
            pinValue = !pinValue;
            PIN_setOutputValue( greenLedHandle0, Board_LED0, pinValue );
            feed++;
            System_printf("Ruuan arvo nyt: %d\n", feed);
            sprintf(feeding, "EAT:%d", 1);
            System_printf("%s\n", feeding);
            System_printf("x-liike \n");
        } else if(fabs(ay) > 0.2 && fabs(ay) < 1.5 && fabs(ax) < 0.2) {
            uint_t pinValue = PIN_getOutputValue( Board_LED1 );
            pinValue = !pinValue;
            PIN_setOutputValue( redLedHandle1, Board_LED1, pinValue );
            Task_sleep(200000/Clock_tickPeriod);
            pinValue = !pinValue;
            PIN_setOutputValue( redLedHandle1, Board_LED1, pinValue );
            exercise++;
            System_printf("Ulkoulun arvo nyt: %d\n", exercise);
            sprintf(exercising, "EXERCISE:%d", 1);
            System_printf("%s\n", exercising);
            System_printf("y-liike \n");
        } else if((fabs(az) > 0.0 && fabs(az) < 0.9) || (fabs(az) > 1.1 && fabs(az) < 1.5)) {
            uint_t pinValue = PIN_getOutputValue( Board_LED0 );
            pinValue = !pinValue;
            PIN_setOutputValue( greenLedHandle0, Board_LED0, pinValue );
            Task_sleep(200000/Clock_tickPeriod);
            pinValue = !pinValue;
            PIN_setOutputValue( greenLedHandle0, Board_LED0, pinValue );
            pet++;
            System_printf("Silityksen arvo nyt: %d\n", pet);
            sprintf(petting, "PET:%d", 1);
            System_printf("%s\n", petting);
            System_printf("z-liike \n");
        }

        Task_sleep(200000 / Clock_tickPeriod);
        //System_flush();
    }

    /*
    // MPU ask data
    int j;
    for(j = 0; j < 101; j++) {
       float ax, ay, az, gx, gy, gz;
       mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
       sensor_data[0][j] = j;
       sensor_data[1][j] = ax;
       sensor_data[2][j] = ay;
       sensor_data[3][j] = az;
       sensor_data[4][j] = gx;
       sensor_data[5][j] = gy;
       sensor_data[6][j] = gz;
       Task_sleep(100000 / Clock_tickPeriod);
     }
    int i, k;
    printf("aika\tacc_x\tacc_y\tacc_z\tgyro_x\tgyro_y\tgyro_z");
    for(i = 0; i < 101; i++){
        //System_printf()
        printf("\n");
        for(k = 0; k < 7; k++){
            //System_printf()
            printf("%.2f\t", sensor_data[k][i]);
        }
    }
    */
        // Sleep 100ms

    // Program never gets here..
    // MPU close i2c
    // MPU power off
    // PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}
Void commTaskFxn(UArg arg0, UArg arg1) {

   char payload[16]; // viestipuskuri
   uint16_t senderAddr;

   // Radio alustetaan vastaanottotilaan
   int32_t result = StartReceive6LoWPAN();
   if(result != true) {
      System_abort("Wireless receive start failed");
   }

   // Vastaanotetaan viestejä loopissa
   while (true) {

        // HUOM! VIESTEJÄ EI SAA LÄHETTÄÄ TÄSSÄ SILMUKASSA
        // Viestejä lähtee niin usein, että se tukkii laitteen radion ja
        // kanavan kaikilta muilta samassa harjoituksissa olevilta!!

        // jos true, viesti odottaa
        if (GetRXFlag()) {

           // Tyhjennetään puskuri (ettei sinne jäänyt edellisen viestin jämiä)
           memset(payload,0,16);
           // Luetaan viesti puskuriin payload
           Receive6LoWPAN(&senderAddr, payload, 16);
           // Tulostetaan vastaanotettu viesti konsoli-ikkunaan
           System_printf(payload);
           System_flush();
           }
      }//HUOM NO TASK_SLEEP
}

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {

    char payload[16] = "ping";
    Send6LoWPAN(IEEE80154_SERVER_ADDR, payload, strlen(payload));

    StartReceive6LoWPAN();
}


int main(void) {

    Task_Handle task;
    Task_Params taskParams;
    Task_Handle commTask;
    Task_Params commTaskParams;

    Board_initGeneral();
    Board_initI2C();
    Init6LoWPAN();

    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    task = Task_create((Task_FuncPtr)sensorFxn, &taskParams, NULL);
    if (task == NULL) {
        System_abort("Task create failed!");
    }
    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority = 1; // Important to set the priority to
    commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
    if (commTask == NULL) {
        System_abort("Task create failed!");
    }



    // Open MPU power pin
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }
    greenLedHandle0 = PIN_open(&ledState0, ledConfig0);
       if(!greenLedHandle0) {
          System_abort("Error initializing LED pins\n");
       }
    redLedHandle1 = PIN_open(&ledState1, ledConfig1);
       if(!redLedHandle1) {
          System_abort("Error initializing LED pins\n");
       }
    buttonHandle = PIN_open(&buttonState, buttonConfig);
      if(!buttonHandle) {
          System_abort("Error initializing button pins\n");
       }
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
          System_abort("Error registering button callback function");
        }

    System_printf("Hello World\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();


    return (0);
}
