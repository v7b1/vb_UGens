
s = Server.default = Server.local.boot;
s = Server.default = Server.internal.boot;

(
{
	var trig = Dust.ar(1); //Impulse.ar(1);
	var input = VBSlide.ar(trig, 1, 100);
	var pit = Latch.ar(WhiteNoise.ar(), trig).range(30, 60);

	//var input = PinkNoise.ar(0.2);
	MiRings.ar(input, 0, pit, 0.2, 0.7, 0.9, bypass: 0)
}.play
)


(
{
	var trig = Dust.ar(1); //Impulse.ar(1);
	//var input = 0;
	var pit = Latch.ar(WhiteNoise.ar(), trig).range(30, 60);

	var input = PinkNoise.ar(0.2);
	MiRings.ar(input, trig, pit, 0.2, 0.7, 0.9, usetrigger: 1, bypass: 0)
}.play
)


{SinOsc.ar(440, 0, 0.2)}.play



s.options.asOptionsString


//-u 57110 -a 1026 -i 4 -o 2 -Z 64 -m 262144 -H "Fireface UC Mac (22946722)" -R 0 -l 1



Server.default = s = Server.remote(\debug, NetAddr("127.0.0.1", 57110));



