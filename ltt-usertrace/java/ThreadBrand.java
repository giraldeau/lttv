// The ThreadBrand.java file
public class ThreadBrand
{
  // Declaration of the Native (C) function
  private static native void trace_java_generic_thread_brand(String arg);
  static {
    System.loadLibrary("ltt-java-thread_brand");
  }
}
