// https://vboehm.net

Audrey : MultiOutUGen {
	*ar { arg in1=0, in2=0, freq=140.0, fb_gain=0.6, body=0.5, hp=60, lp=8000, drywet=0.2, decay=0.5, drive=0.4, echo_send=0.1, echo_time=0.5, echo_fb=0.5;
		^this.multiNew('audio', in1, in2, freq, fb_gain, body, hp, lp, drywet, decay, drive, echo_send, echo_time, echo_fb);
	}


	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

	checkInputs {
		if ( inputs.at(0).rate == 'control', {
			^("input is control rate:" + inputs.at(0) + inputs.at(0).rate);
		});
		if ( inputs.at(1).rate == 'control', {
			^("input is control rate:" + inputs.at(1) + inputs.at(1).rate);
		});
		^this.checkValidInputs;
	}

}