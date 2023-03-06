
String getTimeNTP(){
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();
//  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  // Extract time
  return timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  
}
