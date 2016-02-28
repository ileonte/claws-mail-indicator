OBJS       := $(patsubst %.cpp, %.cpp.o, $(SRCS))
OBJS       := $(patsubst %.cc, %.cc.o, $(OBJS))
OBJS       := $(patsubst %.c, %.c.o, $(OBJS))
ACTUALOBJS := $(addprefix $(BUILDDIR)/,$(OBJS))
DEPS       := $(patsubst %.o, %.d, $(ACTUALOBJS))

$(TARGET): $(LIBDIR)/$(TARGET)

$(LIBDIR)/$(TARGET): $(ACTUALOBJS)
	$(shell mkdir -p $(dir $@))
	$(LN) -shared $(LDFLAGS) -o $@ $(ACTUALOBJS) $(LDFLAGS) $(LIBS)

-include $(DEPS)

$(BUILDDIR)/%.c.o: $(CURDIR)/%.c
	$(shell mkdir -p $(dir $@))
	$(CC) $(CFLAGS) -MMD -o $@ -c $<

$(BUILDDIR)/%.cc.o: $(CURDIR)/%.cc
	$(shell mkdir -p $(dir $@))
	$(CXX) $(CXXFLAGS) -MMD -o $@ -c $<

$(BUILDDIR)/%.cpp.o: $(CURDIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	$(CXX) $(CXXFLAGS) -MMD -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(LIBDIR)/$(TARGET) $(ACTUALOBJS) $(DEPS)
