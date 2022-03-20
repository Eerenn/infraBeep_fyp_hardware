//IR rawlen signal is constuct 
void getArray(String rawlen) {
    while(sentencePost < rawlen.length()) {
    if(rawlen.charAt(sentencePost) == ',') {
      val.trim();
      arr[count] = val.toInt();
      sentencePost++;
      count++;
      val="";
    } else {
      val += rawlen.charAt(sentencePost);
      sentencePost++;
    }
  }
      arr[count] = val.toInt();
}
