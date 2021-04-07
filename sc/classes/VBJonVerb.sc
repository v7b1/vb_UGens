// https://vboehm.net

VBJonVerb : MultiOutUGen {
	*ar { arg in, decay=0.6, damping=0.3, inputbw=0.8, erfl=0.5, tail=0.5;
		^this.multiNew('audio', in, decay, damping, inputbw, erfl, tail)
	}


	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}