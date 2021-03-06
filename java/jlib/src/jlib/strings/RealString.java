package jlib.strings;

import java.lang.String;
import jlib.*;
import jlib.strings.*;

public class RealString extends SomeString {

  private String s;

  public static int realcnt=0;

  public RealString(String ss) {
    realcnt++;
    s=ss;
  }

  public char charAt(int i) {
    return s.charAt(i);
  }

  public String real() {
    return s;
  }
  public String toString() {
    return s;
  }

	public int length() {
		return s.length();
	}

  public String topString() {
    return s;
  }
  public int topIndex(int i) {
    return i;
  }

	public int indexOf(char c,int x) {
		return s.indexOf(c,x);
	}
	public int indexOf(String ss,int x) {
		return s.indexOf(ss,x);
	}

}
