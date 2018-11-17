h_sources = CLExec.h RuleExecution.h RuleParser.h TranElemLiterals.h 
cc_sources = BeamSearch.cpp CLExec.cpp Main.cpp RuleExecution.cpp RuleParser.cpp RulesApplier.cpp YasmetFormatter.cpp

#library_include_HEADERS = $(h_sources)

bin_PROGRAMS = apertium-ambiguous

apertium_ambiguous_SOURCES = $(cc_sources)
apertium_ambiguous_LDFLAGS = -lpugixml
