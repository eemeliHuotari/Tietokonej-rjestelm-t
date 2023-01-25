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

#define STACKSIZE 2048
Char taskStack[STACKSIZE];

// MPU power pin global variables
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;

// MPU power pin
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

float sensor_data[7][100];

Void sensorFxn(UArg arg0, UArg arg1) {


    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    // Note the different configuration below
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 200ms for the MPU sensor to power up
    Task_sleep(200000 / Clock_tickPeriod);
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


    // MPU ask data
    int j;
    for(j = 0; j < 101; j++) {
       float ax, ay, az, gx, gy, gz;
       mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
       sensor_data[0][j] = Clock_getTicks()/100000.0;
       sensor_data[1][j] = ax;
       sensor_data[2][j] = ay;
       sensor_data[3][j] = az;
       sensor_data[4][j] = gx;
       sensor_data[5][j] = gy;
       sensor_data[6][j] = gz;
       Task_sleep(100000 / Clock_tickPeriod);
     }

    int i, k;
    printf("aika\tacc_x,\tacc_y,\tacc_z,\tgyro_x,\tgyro_y,\tgyro_z");
    for(i = 0; i < 101; i++){
        printf("\n");
        for(k = 0; k < 7; k++){
            printf("%.2f\t", sensor_data[k][i]);
        }
    }
        // Sleep 100ms

    // Program never gets here..
    // MPU close i2c
    I2C_close(i2cMPU);
    // MPU power off
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}


int main(void) {

    Task_Handle task;
    Task_Params taskParams;

    Board_initGeneral();
    Board_initI2C();

    // Open MPU power pin
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }

    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    task = Task_create((Task_FuncPtr)sensorFxn, &taskParams, NULL);
    if (task == NULL) {
        System_abort("Task create failed!");
    }

    System_printf("Hello World\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();


    return (0);
}