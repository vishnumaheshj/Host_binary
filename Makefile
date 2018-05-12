znp_host:
	$(MAKE) -C examples/dataSendRcv/build/gnu
	cp examples/dataSendRcv/build/gnu/dataSendRcv.bin znp_host

clean:
	$(MAKE) -C examples/dataSendRcv/build/gnu clean
	rm znp_host

