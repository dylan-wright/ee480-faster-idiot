all:
	iverilog -o pipe pipe.v
	./pipe
covrep:
	covered score -t bench -v pipe.v  -I . -vcd dump.vcd -o covdb
	covered report -d v covdb > covrep

clean:
	rm pipe dump.vcd
