VBPVoc : MultiOutUGen {

	*ar {
		arg numChannels, bufnum, playpos = 0, mul = 1.0, add = 0.0;
		//^this.multiNew('audio', bufnum, playpos).madd(mul, add)
		//numChannels = bufnum.numChannels;
		^this.multiNew('audio', numChannels, bufnum, playpos).madd(mul, add)
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg argNumChannels ... theInputs;
		inputs = theInputs;  // das heisst, argNumChannels wird nicht an UGen uebergeben!
		^this.initOutputs(argNumChannels, rate);
	}

	/*
	*calcBufSize {arg fftsize, overlap, inputbuffer;
		var hopsize = fftsize.div(overlap);
		^fftsize * ((inputbuffer.numFrames - fftsize) / hopsize + 2).roundUp;
	}*/
/*
	*createBuffer {arg server, fftsize, overlap, inputbuffer;
		var hopsize = fftsize.div(overlap);
		var numframes = fftsize.div(2) * ((inputbuffer.numFrames - fftsize)
			/ hopsize + 2).roundUp;

		^Buffer.alloc(server, numframes, inputbuffer.numChannels * 2);
	}
	*/
	*createBuffer {arg server, fftsize, inputbuffer;
		var overlap = 4;
		var hopsize = fftsize.div(overlap);
		var fftframes = ((inputbuffer.numFrames - fftsize) / hopsize + 2).roundUp;
		var outframes = fftsize.div(2) * fftframes;
		postln("inputframes: "+inputbuffer.numFrames);
		postln("outframes: "+outframes);
		postln("fftframes: "+fftframes);
		^Buffer.alloc(server, outframes, inputbuffer.numChannels * 2);
	}
}



+ Buffer {

	pvocAnal { arg buf, fftsize=2048;
		server.listSendMsg(["/b_gen", bufnum, "PVocAnal", buf.bufnum, fftsize, 4]);
	}

	freezeBuf { arg magthresh=0.0;
		server.listSendMsg(["/b_gen", bufnum, "FreezeBuf", magthresh]);
	}

}