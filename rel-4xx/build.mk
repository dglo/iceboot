# Build.mk file for the iceboot project.

BUILD_FLAVOR := c

../tools/resources/build-body.mk: ;

include ../tools/resources/build-body.mk

%: force
	@$(MAKE) -f $(MAKEFILE_TO_USE) PLATFORM="$(PLATFORM)" ICESOFT_HOST="$(ICESOFT_HOST)" ICESOFT_BUILD="$(ICESOFT_BUILD)" PROJECT="$(PROJECT)" PROJECT_TAG="$(PROJECT_TAG)" $(@)

clean:
	@$(MAKE) -f $(MAKEFILE_TO_USE) PLATFORM="$(PLATFORM)" ICESOFT_HOST="$(ICESOFT_HOST)" ICESOFT_BUILD="$(ICESOFT_BUILD)" PROJECT="$(PROJECT)" PROJECT_TAG="$(PROJECT_TAG)" BUILT_FILES="" BUILD_DEPENDS="" $(@)
