$(TARGET):$(DEP)
	$(CC) -o $@ $(src) $(addprefix -l,$(lib)) $(CC_FLAG)
