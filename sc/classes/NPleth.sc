// https://vboehm.net

NPleth : UGen {
	*ar {
		arg algo=0, x=0.5, y=0.5, cf = 0.75, res = 0, filtMode = 0;
		^this.multiNew('audio', algo, x, y, cf, res, filtMode);
	}
	*kr {
		arg algo=0, x=0.5, y=0.5, cf = 0.75, res = 0, filtMode = 0;
		^this.multiNew('control', algo, x, y, cf, res, filtMode);
	}
}
