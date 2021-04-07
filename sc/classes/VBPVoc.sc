// https://vboehm.net, 2019

VBPVoc : MultiOutUGen {

	*ar {
		arg numChannels, bufnum, playpos = 0, fftsize=2048, mul = 1.0, add = 0.0;
		^this.multiNew('audio', numChannels, bufnum, playpos, fftsize).madd(mul, add)
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg argNumChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(argNumChannels, rate);
	}


	*createBuffer {arg server, fftsize, inputbuffer;
		var overlap = 4;
		var hopsize = fftsize.div(overlap);
		var fftframes = ((inputbuffer.numFrames - fftsize) / hopsize + 2).roundUp;
		var outframes = fftsize.div(2) * fftframes;
		//postln("inputframes: "+inputbuffer.numFrames);
		//postln("outframes: "+outframes);
		postln("fftframes: "+fftframes);
		^Buffer.alloc(server, outframes, inputbuffer.numChannels * 2);
	}
}



+ Buffer {
	// Todo: add a completion message
	pvocAnal { arg buf, fftsize=2048, completionMessage;
		server.listSendMsg(["/b_gen", bufnum, "PVocAnal", buf.bufnum, fftsize, completionMessage.value(this)]);
	}

/*	freezeBuf { arg magthresh=0.0;
		server.listSendMsg(["/b_gen", bufnum, "FreezeBuf", magthresh]);
	}*/

}