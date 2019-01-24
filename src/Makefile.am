
h_sources = CLExec.h RuleExecution.h RuleParser.h TranElemLiterals.h 
cc_sources = CLExec.cpp RuleExecution.cpp RuleParser.cpp 

#library_include_HEADERS = $(h_sources)

bin_PROGRAMS = beam-search yasmet-formatter rules-applier beam-result model-result

beam_search_SOURCES = BeamSearch.cpp $(cc_sources)
beam_search_LDFLAGS = -lpugixml  -licuuc -licuio 

yasmet_formatter_SOURCES = YasmetFormatter.cpp $(cc_sources)
yasmet_formatter_LDFLAGS = -lpugixml  -licuuc -licuio 

rules_applier_SOURCES = RulesApplier.cpp $(cc_sources)
rules_applier_LDFLAGS = -lpugixml  -licuuc -licuio 

beam_result_SOURCES = BeamResult.cpp $(cc_sources)
beam_result_LDFLAGS = -lpugixml  -licuuc -licuio 

model_result_SOURCES = ModelResult.cpp $(cc_sources)
model_result_LDFLAGS = -lpugixml  -licuuc -licuio 
