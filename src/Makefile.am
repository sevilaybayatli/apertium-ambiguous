
h_sources = CLExec.h RuleExecution.h RuleParser.h TranElemLiterals.h 
cc_sources = CLExec.cpp RuleExecution.cpp RuleParser.cpp 

bin_PROGRAMS = beam-search yasmet-formatter rules-applier langMod-analysis order-ambig-sents comb-align

beam_search_SOURCES = BeamSearch.cpp $(cc_sources)
beam_search_LDFLAGS = -lpugixml -lapertium3

yasmet_formatter_SOURCES = YasmetFormatter.cpp $(cc_sources)
yasmet_formatter_LDFLAGS = -lpugixml -lapertium3

rules_applier_SOURCES = RulesApplier.cpp $(cc_sources)
rules_applier_LDFLAGS = -lpugixml -lapertium3 
	
langMod_analysis_SOURCES= LangModAnalysis.cpp $(cc_sources)
langMod_analysis_LDFLAGS= -lpugixml -lapertium3

order_ambig_sents_SOURCES= OrderAmbigSents.cpp $(cc_sources)
order_ambig_sents_LDFLAGS= -lpugixml -lapertium3

comb_align_SOURCES= CombAlign.cpp $(cc_sources)
comb_align_LDFLAGS= -lpugixml -lapertium3
