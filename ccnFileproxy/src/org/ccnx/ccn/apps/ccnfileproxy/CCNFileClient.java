package org.ccnx.ccn.apps.ccnfileproxy;

import java.io.IOException;

import org.ccnx.ccn.config.ConfigurationException;
import org.ccnx.ccn.protocol.ContentName;
import org.ccnx.ccn.protocol.Interest;
import org.ccnx.ccn.protocol.MalformedContentNameStringException;

public class CCNFileClient {

	public static void main(String[] args) throws MalformedContentNameStringException, ConfigurationException, IOException {

		if (args.length < 1) {

			return;
		}

		
		String ccnURI = args[1];
		String filePrefix=args[0];	
		String fileName=args[2];
		
		
		
		Interest interest=Interest.constructInterest(ContentName.fromURI(ccnURI+"/"+fileName), null, null, null, null, null);
		CCNFileProxy fp=new CCNFileProxy(filePrefix,ccnURI);
		
		fp.handleInterest(interest);
		
		
		

	}
}
