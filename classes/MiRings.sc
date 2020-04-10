// vboehm.net, 2020

MiRings : MultiOutUGen {

	*ar {
		arg in, trigger=0.0, pitch=60.0, structure=0.25, brightness=0.7, damping=0.7, position=0.3, usetrigger=0, bypass=0, model=0, polyphony=1, mul=1.0, add=0;
		^this.multiNew('audio', in, trigger, pitch, structure, brightness, damping, position, usetrigger, bypass, model, polyphony).madd(mul, add);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}