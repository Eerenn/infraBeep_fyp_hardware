
//decode IR signal into raw string
String  dumpCode (decode_results *results)
{
  String rawVal = ""; //declare empty string
  Serial.println("");

  // Dump data
  for (int i = 1;  i < results->rawlen;  i++) {
    rawVal += String(results->rawbuf[i] * USECPERTICK,DEC);
    if ( i < results->rawlen-1 ) rawVal += ","; 
  }
  return rawVal;
}
