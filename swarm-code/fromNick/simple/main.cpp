#if PROGMEM_SIZE > 0x010000
#define PGM_READ_BYTE pgm_read_byte_far
#else
#define PGM_READ_BYTE pgm_read_byte_near
#endif

#include "main.h"
#include "board_init.c"
#include "sonar.c"
#include "swarmfunctions.c"
#include "communication.c"

void Calib();


// ============================================================================================
// Timer tick ISR (1 kHz)
// ============================================================================================
ISR(TCC0_OVF_vect)
{
    jiffies++;	// Timers
    
    // update every 1/100th of a second
    if(jiffies%10 == 0)
    {
        // update the servo on this cycle
        if (updateRate == SMOOTH){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
        if (inTransition)
        {
            crossFade += fadeIncrement; // should take 1/2 second
            if (crossFade >= 1.0)
            {
                crossFade = 1.0;
                inTransition = false;
            }
        }
    }
    
    // 50Hz update rate
    if(jiffies%20 == 0)
    {
        counterFiftyHz++;
        
        beatCounterFiftyHz++;
        beatCounterFiftyHz %= (ticksPerBeat * numBeats);
        
        send_neighbor_data(curAngle, myStrength);
        
        // storing as unsigned ints, when used must subtract 90
        neighborBuffer[ABOVE][neighborBufferPtr] = (uint8_t)(neighborAngles[ABOVE] + 90);
        neighborBuffer[BELOW][neighborBufferPtr] = (uint8_t)(neighborAngles[BELOW] + 90);
        neighborBuffer[LEFT][neighborBufferPtr] = (uint8_t)(neighborAngles[LEFT] + 90);
        neighborBuffer[RIGHT][neighborBufferPtr] = (uint8_t)(neighborAngles[RIGHT] + 90);
        
        // increment pointer,  check for roll over
        neighborBufferPtr++;
        neighborBufferPtr %= NEIGHBOR_BUFFER_SIZE;
    }
    
    
    // every 100 ms
    if(jiffies%100 == 0)
    {
        // increment 10 hz counter
        counterTenHz++;
        ////////////////////////////////////////////////////
        // Read analog input
        ////////////////////////////////////////////////////
        if (ADCB.CH0.INTFLAGS) {
            //ADCB.CH0.INTFLAGS = ADC_CH__CHIF_bm;
            ADCB.CH0.INTFLAGS = 0x01;
            light_sensor = ADCB_CH0_RES;
        }
            
        if (ADCA.CH0.INTFLAGS){
            // if on the bottom row assign sensor value here, others will read from below
            if(bottom)
                sensor_value = ADCA_CH0_RES;
            
            ADCA.CH0.INTFLAGS = 0x01;
        }
        
        // update buffer
        sensorBuf[sensorBufPtr++] = sensor_value;
        sensorBufPtr %= sensorBufSize;
        
        // update presenseDetected variable
        // store what happened last
        presenceDetectedLast = presenceDetected;
        neighborPresenceDetectedLast = neighborPresenceDetected;
        presenceDetected = true;
        // check if current or recent are all above thresh, else set to false
        for(int i = 0; i < sensorBufSize; i++){
            // if any in the recent buffer are below threshold set to false
            if(presenceDetectedLast){
                if (sensorBuf[i] < PRESENCE_OFF_THRESH)
                    presenceDetected = false;
            }
            else{
                if (sensorBuf[i] < PRESENCE_THRESH)
                    presenceDetected = false;
            }
        }
        
        if (neighborData[LEFT].strength > STRENGTH_THRESHOLD || neighborData[RIGHT].strength > STRENGTH_THRESHOLD){
            neighborPresenceDetected = true;
        }
        else
            neighborPresenceDetected = false;
        
        if (presenceDetected != presenceDetectedLast){
            newPresence = true;
            presenceTimer = 0;
        }
        
        // if presence is moving around in system reset timer
        if (neighborPresenceDetected != neighborPresenceDetectedLast){
            newNeighborPresence = true;
            neighborPresenceTimer = 0;
        }
        
        if (presenceDetected){
            myStrength = 1.0;
            strengthDir = MOOT;
        }
                    
        else{
            if(((neighborData[LEFT].strength > STRENGTH_THRESHOLD) || (neighborData[RIGHT].strength > STRENGTH_THRESHOLD)) && neighborData[LEFT].strength != neighborData[RIGHT].strength)
            {
                if (neighborData[LEFT].strength > neighborData[RIGHT].strength && (neighborData[LEFT].fromDir == LEFT ||neighborData[LEFT].fromDir == MOOT)){
                    myStrength = neighborData[LEFT].strength / 1.3;
                    strengthDir = LEFT;
                    lastStrengthDir = LEFT;
                    lastStrength = myStrength;
                }
                else if (neighborData[RIGHT].strength > neighborData[LEFT].strength && (neighborData[RIGHT].fromDir == RIGHT || neighborData[RIGHT].fromDir == MOOT)){
                    myStrength = neighborData[RIGHT].strength / 1.3;
                    strengthDir = RIGHT;
                    lastStrengthDir = RIGHT;
                    lastStrength = myStrength;
                }
            }
            else
            {
                myStrength = 0.0;
                strengthDir = MOOT;
            }
        }
        
        if (neighborPresenceDetected){
            if (neighborPresenceTimer > neighborPresenceTimeOut){
                // the first time through here start a cross fade
                fprintf_P(&usart_stream, PSTR("neiP timeOut\r\n"));
                if(!ignoreNeighborPresence)
                    startXFade(0.02);
                ignoreNeighborPresence = true;
            }
        }
        else
            ignoreNeighborPresence = false;
        
        if (presenceDetected){
            if (presenceTimer > presenceTimeOut){
                fprintf_P(&usart_stream, PSTR("P timeOut\r\n"));
                if(!ignorePresence)
                    startXFade(0.02);
                ignorePresence = true;
            }
        }
        else{
            ignorePresence = false;
        }
    } // \(jiffies % 100==0)
    
    
    // every 100 ms
    if(jiffies%100 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == ONE_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
    }
    
    // every 200 ms
    if(jiffies%200 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == TWO_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
        //fprintf_P(&usart_stream, PSTR("my: %f, d.%i, l%f, r%f\r\n"), myStrength, strengthDir, neighborData[LEFT].strength, neighborData[RIGHT].strength);
        

            currentBeat++;
            currentBeat %= numBeats;

    }
    // every 300 ms
    if(jiffies%300 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == THREE_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
    }
    
    // every 400 ms
    if(jiffies%400 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == FOUR_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
    }
    // every 600 ms
    if(jiffies%600 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == SIX_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
    }
    // every 800 ms
    if(jiffies%800 == 0)
    {
        display_on = true;
        
        // update the servo on this cycle
        if (updateRate == EIGHT_HUNDRED){
            servo_motor_on   = true;
            sendmessage_fast = true;
        }
    }
    
    // 1 second
    if(jiffies%1000 == 0)
    {
        //if(communication_on) sec_counter++;
        sec_counter++;
        presModeCounter++;
        if (presenceDetected){
            presenceTimer++;
        }
        if(neighborPresenceDetected)
            neighborPresenceTimer++;
        
        //sync = true;		//synchro bit should be set every 1 sec
        //rhythm_on = true;
                
        if(!communication_on) LED_PORT.OUT =  LED_USR_0_PIN_bm;
        if(communication_on)  LED_PORT.OUT = !LED_USR_0_PIN_bm;
        
        if(debugPrint){
            fprintf_P(&usart_stream, PSTR("cur angle: %f\r\n"), curAngle);
            
            if(debugPrint)
                fprintf_P(&usart_stream, PSTR("A0: %u\r\n"), sensor_value);
        }
    }
    

    // every 2 seconds
    if (jiffies%2000 == 0)
    {
        if (swarm_id != _MAIN_BOARD)
        {
            calib_switch  = true ;
        }
        if (swarm_id == _MAIN_BOARD)
        {
//            asm("nop");
//            asm("nop");
            Calib();
        }
    }
    
    // check every 5 seconds if it has recieved messages
    if(jiffies%5000 == 0){
        // no neighbor
        if(!connected[BELOW] && !connected[LEFT] && connected[ABOVE] && connected[RIGHT]){
            special = true;
            bottom = true;
            fprintf_P(&usart_stream, PSTR("I'm special\r\n"));  
        }
        // no neighbor from below // yes neighbor above
        if(!connected[BELOW] && connected[ABOVE]) bottom = true;
                
        numConnected = 0;
        for(int i = 0; i < 6; i++){
            if (connected[i])
                numConnected++;
        }
    
    }
   	xgrid.process();
    
    // cycle through all the current modes
    // changing every cycleLength seconds
    int cycleLength = 10;
    int presenceCycleLength = 5;
    if(cycleOn){
        if (sec_counter >= cycleLength){
            currentMode++;
            if(currentMode > BREAK)
                currentMode = 0;
            sec_counter = 0;
            
            fprintf_P(&usart_stream, PSTR("currentMode chaged to: %i, \r\n"), currentMode);
        }
        
        if (presModeCounter >= presenceCycleLength){
            presMode++;
            if(presMode > IGNORE)
                presMode = 0;
            presModeCounter = 0;
            
            fprintf_P(&usart_stream, PSTR("presMode chaged to: %i, \r\n"), presMode);
        }
    }
}

void StageInit(int StageTime, const char str[])
{
	if(sec_counter == StageTime && special)
	{
		send_message(MESSAGE_COMMAND, ALL_DIRECTION, ALL, str);
		init_variables();
	}
}



// ============================================================================================
// MAIN FUNCTION
// ============================================================================================
int main(void)
{
	xgrid.rx_pkt = &rx_pkt;
    
	_delay_ms(50);
    
	// ========== INITIALIZATION ==========
    init();				//for board
	init_servo();		//for servo
	init_variables();	//for program
	init_sonar();		//for sensor
    
	fprintf_P(&usart_stream, PSTR("START (build number : %ld)\r\n"), (unsigned long) &__BUILD_NUMBER);
    //printKeyCommands();
    
    srand(swarm_id);
    randomPeriod = (18.0 * rand() / RAND_MAX)+ 2.0;
    
    // START ADC
    //
    ADCA.CTRLA |= ADC_CH0START_bm;
    ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm;
	// ===== SONAR CHECK & Indicated by LED (attached/not = GREEN/RED) =====
	sonar_attached = check_sonar_attached();	//1:attached, 0:no
    
	if(sonar_attached)	{LED_PORT.OUT = LED_USR_2_PIN_bm; _delay_ms(2000);}
	else				{LED_PORT.OUT = LED_USR_0_PIN_bm; _delay_ms(2000);}
    
	// ===== Identification of Left Bottom Corner module =====
	// Special module is necessary
	// 1) as a pace maker in "rhythm" mode,
	// 2) as a messanger of variable-reset signal
	temp_time = jiffies + 2000;
	while(jiffies < temp_time)
	{
		// send dummy data
		if(sendmessage_fast)
		{
			send_message(MESSAGE_COMMAND, ALL_DIRECTION, ALL, "@");
			sendmessage_fast = false;
		}
	}
    
	// #################### Synchronize ####################
    calib_switch = true;
    calib_double_switch = true;
    calib_times = 0;
    calibinfo._calib_times = 0;
    
    if (swarm_id == _MAIN_BOARD)
    {
        _delay_ms(1000);
        Calib();
        special = true;
    }
    
    
    // #################### MAIN LOOP ####################

	while (1)
	{
		// ========== REBOOT PROCESS ==========
		if(reboot_on)
		{
			temp_time = jiffies + 3000;
			//while(jiffies < temp_time){LED_PORT.OUTTGL = LED_USR_1_PIN_bm; _delay_ms(100);}
			xboot_reset();
		}
        
		// ========== KEY INPUT ==========
		key_input();
                
        ////////////////////////////////////////////////
        // if angle is being updated this cycle
        if(servo_motor_on)
        {
            servoBehavior();
            lastMode = currentMode;
            lastPresMode = presMode;
            
            // wait until updateRate has come back around
            servo_motor_on = false;
            
        }
        /////////////////////////////////////////////////////
	}	
	return 0;
}



void Calib()
{
    
    Xgrid::Packet pkt;
	pkt.type = _TIME_CALIB;
	pkt.flags = 0;
	pkt.radius = 1;
	
	calibinfo._calib_times += 1;
	calibinfo._jiffies = jiffies;
	pkt.data = (uint8_t *) &calibinfo;
    
	pkt.data_len = sizeof(CalibInfo);
	
	xgrid.send_packet(&pkt,0b00111111);
    LED_PORT.OUTTGL = LED_USR_1_PIN_bm;

}


