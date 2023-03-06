String functelur(){
  val = digitalRead(2);   // read sensor value
  if (val == HIGH) {           // check if th
    delay(100);                // delay 100 milliseconds 
    
    if (state == LOW) {
      datatelur = "Detected";
      Serial.println("Motion detected!"); 
      state = HIGH;       // update variable state to HIGH
    }
  } 
  else {
      delay(200);             // delay 200 milliseconds 
      
      if (state == HIGH){
        datatelur = "Stopped";
        Serial.println("Motion stopped!");
        state = LOW;       // update variable state to LOW
    }
  }
  return datatelur;
}
