
h_sources = CLExec.h RuleExecution.h RuleParser.h TranElemLiterals.h 
cc_sources = CLExec.cpp RuleExecution.cpp RuleParser.cpp 

#library_include_HEADERS = $(h_sources)

bin_PROGRAMS = beam-search yasmet-formatter rules-applier best-langmod langMod-analysis

beam_search_SOURCES = BeamSearch.cpp $(cc_sources)
beam_search_LDFLAGS = -lpugixml  -licuuc -licuio 

yasmet_formatter_SOURCES = YasmetFormatter.cpp $(cc_sources)
yasmet_formatter_LDFLAGS = -lpugixml  -licuuc -licuio 

rules_applier_SOURCES = RulesApplier.cpp $(cc_sources)
rules_applier_LDFLAGS = -lpugixml  -licuuc -licuio 

#beam_result_SOURCES = BeamResult.cpp $(cc_sources)
#beam_result_LDFLAGS = -lpugixml  -licuuc -licuio 

best_langmod_SOURCES= BestLangMod.cpp $(cc_sources)
best_langmod_LDFLAGS= -lpugixml  -licuuc -licuio 
	
langMod_analysis_SOURCES= LangModAnalysis.cpp $(cc_sources)
langMod_analysis_LDFLAGS= -lpugixml  -licuuc -licuio

