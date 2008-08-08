import java.io.*;

public class read_trace {
	public static void main(String[] args) {
		int tmp = 0;

		try {
			FileInputStream file_input = new FileInputStream ("../trace.dat");
			DataInputStream data_input = new DataInputStream (file_input);
			
			int timestamp;
			short id;
			byte length;

			while(true) {
				timestamp = data_input.readInt();
				id = data_input.readShort();
				length = data_input.readByte();

				byte[] buf = new byte[length];
				data_input.readFully(buf, 0, length);


				ByteArrayInputStream eargs = new ByteArrayInputStream(buf);
				DataInputStream eargs_data = new DataInputStream (eargs);

				/* read arg 1 (short) */
				short arg1 = eargs_data.readShort();

				/* read arg 2 (string) */
				eargs_data.mark(10000);
				int strlen=0;
				while(eargs_data.readByte() != 0)
					strlen++;
				eargs_data.reset();
				byte[] arg2 = new byte[strlen];
				eargs_data.readFully(arg2, 0, strlen);

				if(args.length>0 && args[0].equals("-p"))
					System.out.printf("timestamp %d id %d args=(short=%d string=\"%s\") %n", timestamp, id, arg1, new String(arg2));
			}

		}
		catch(IOException e) {
			//System.out.println ("IO exception = " + e );
			//e.printStackTrace();
		}

		//file_input.close();
	}
}
