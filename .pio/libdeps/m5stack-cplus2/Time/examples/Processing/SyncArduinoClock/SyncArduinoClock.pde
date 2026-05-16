/**
 * SyncArduinoClock. 
 *
 * SyncArduinoClock is a Processing sketch that responds to Arduino
 * requests for time synchronization messages.  Run this in the
 * Processing environment (not in Arduino) on your PC or Mac.
 *
 * Download TimeSerial onto Arduino and you should see the time
 * message displayed when you run SyncArduinoClock in Processing.
 * The Arduino time is set from the time on your computer through the
 * Processing sketch.
 *
 * portIndex must be set to the port connected to the Arduino
 *
 * The current time is sent in response to request message from Arduino 
 * or by clicking the display window 
 *
 * The time message is 11 ASCII text characters; a header (the letter 'T')
 * followed by the ten digit system time (unix time)
 */
 

import processing.serial.*;
import java.util.Date;
import java.util.Calendar;
import java.util.GregorianCalendar;

public static final short portIndex = 0;  // select the com port, 0 is the first port
public static final String TIME_HEADER = "T"; //header for arduino serial time message 
public static final char TIME_REQUEST = 7;  // ASCII bell character 
public static final char LF = 10;     // ASCII linefeed
public static final char CR = 13;     // ASCII linefeed
Serial myPort;     // Create object from Serial class

void setup() {  
  size(200, 200);
  println(Serial.list());
  println(" Connecting to -> " + Serial.list()[portIndex]);
  myPort = new Serial(this,Serial.list()[portIndex], 9600);
  println(getTimeNow());
}

void draw()
{
  textSize(20);
  textAlign(CENTER);
  fill(0);
  text("Click to send\nTime Sync", 0, 75, 200, 175);
  if ( myPort.available() > 0) {  // If data is available,
    char val = char(myPort.read());         // read it and store it in val
    if(val == TIME_REQUEST){
       long t = getTimeNow();
       sendTimeMessage(TIME_HEADER, t);   
    }
    else
    { 
       if(val == LF)
           ; //igonore
       else if(val == CR)           
         println();
       else  
         print(val); // echo everying but time request
    }
  }  
}

void mousePressed() {  
  sendTimeMessage( TIME_HEADER, getTimeNow());   
}


void sendTimeMessage(String header, long time) {  
  String timeStr = String.valueOf(time);  
  myPort.write(header);  // send header and time to arduino
  myPort.write(timeStr); 
  myPort.write('\n');  
}

long getTimeNow(){
  // java time is in ms, we want secs    
  Date d = new Date();
  Calendar cal = new GregorianCalendar();
  long current = d.getTime()/1000;
  long timezone = cal.get(cal.ZONE_OFFSET)/1000;
  long daylight = cal.get(cal.DST_OFFSET)/1000;
  return current + timezone + daylight; 
}
