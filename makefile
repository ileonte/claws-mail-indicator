include mk/vars.mk

TARGETLIBS := src
TARGETBINS :=
CLEANTARGETS := $(addsuffix /maketarget-clean, $(TARGETBINS) $(TARGETLIBS))

.PHONY: clean dist-clean $(TARGETBINS) $(TARGETLIBS) $(CLEANTARGETS)
all: $(TARGETLIBS)

$(TARGETBINS): $(TARGETLIBS)
	$(MAKE) -f BUILD.mk -C $@
$(TARGETLIBS):
	$(MAKE) -f BUILD.mk -C $@

dist-clean:
	$(RM) -r $(BUILDBASE)
clean: $(CLEANTARGETS)
$(CLEANTARGETS):
	$(MAKE) -f BUILD.mk -C $(dir $@) clean
