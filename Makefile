OBJS=\
	main.o\
	problem.o\
	verifier.o\
	arguments.o\
	heuristic.o\
	local_search.o\
	solution_info.o\
	batch_verifier.o\
	heuristic_factory.o\
	fast_local_search.o\
	vns.o\
	move_verifier.o\
	simulated_annealing.o\
	best_improvement_local_search.o\
	first_improvement_local_search.o\
	multi_start_local_search.o\
	analyzer.o\
	els.o\
	smart_shaker.o\
	path_relinking.o\
	greedy_advisor.o\
	tabu_search.o\
	vns2.o\
	exchange_verifier.o\
	random_exchange_ls.o\
	random_move_ls.o\
	linear_solver.o\
	parameter_map.o\
	random_local_search_routine.o\
	random_shake_routine.o\
	vns3.o\
	deep_local_search_routine.o\
	smart_local_search_routine.o\
	problem_info.o\
	deep_shake_routine.o\
	SA_local_search_routine.o\
	linear_solver_sub_problem.o\
	load_cost_optimizer.o\
	balance_cost_optimizer.o\
	sequential_local_search_routine.o\
	optimized_local_search_routine.o

OBJ_OPT_FILES=$(patsubst %.o,obj/opt/%.o,$(OBJS))
OBJ_DBG_FILES=$(patsubst %.o,obj/dbg/%.o,$(OBJS))
OBJ_TGT_FILES=$(patsubst %.o,obj/tgt/%.o,$(OBJS))
OBJ_PROF_FILES=$(patsubst %.o,obj/prof/%.o,$(OBJS))

DEP_FILES=$(patsubst %.o,dep/%.d,$(OBJS))

INCLUDE_DIRS=-Iinclude -I/opt/boost
LIBS=-L/opt/boost/stage/lib -lboost_program_options -lrt -lpthread -lglpk

ifeq ($(TOOLS),intel)
CXX=icpc
DEP=icpc
LINK=icpc
LINK_PROF=icpc
DEPFLAGS=-std=c++0x
CXXFLAGS_DBG=-O0 -g -std=c++0x -Wall -Wsign-compare -Wcheck -wd279 $(INCLUDE_DIRS)
CXXFLAGS_OPT=-O2 -xHost -std=c++0x -Wall -Wsign-compare -Wcheck -wd279 $(INCLUDE_DIRS)
CXXFLAGS_TGT=-O2 -march=core2 -std=c++0x -Wall -Wsign-compare -Wcheck -wd279 $(INCLUDE_DIRS)
CXXFLAGS_PROF=-O2 -g -xHost -std=c++0x -Wall -Wsign-compare -Wcheck -wd279 $(INCLUDE_DIRS)
else
CXX=g++
DEP=g++
LINK=g++
LINK_PROF=g++
DEPFLAGS=-std=c++0x
CXXFLAGS_DBG=-O0 -g -std=c++0x -Wall -Wsign-compare $(INCLUDE_DIRS)
CXXFLAGS_OPT=-O3 -march=native -std=c++0x -Wall -Wsign-compare $(INCLUDE_DIRS)
CXXFLAGS_TGT=-O3 -march=core2 -std=c++0x -Wall -Wsign-compare $(INCLUDE_DIRS)
CXXFLAGS_PROF=-O3 -g -march=native -std=c++0x -Wall -Wsign-compare $(INCLUDE_DIRS)
endif

all: opt dbg tgt prof

opt: bin/hybrid_heuristic_opt

dbg: bin/hybrid_heuristic_dbg

tgt: bin/hybrid_heuristic_tgt

prof: bin/hybrid_heuristic_prof

bin/hybrid_heuristic_opt: $(OBJ_OPT_FILES)
	$(LINK) $(OBJ_OPT_FILES) $(LIBS) -o $@

bin/hybrid_heuristic_dbg: $(OBJ_DBG_FILES)
	$(LINK) $(OBJ_DBG_FILES) $(LIBS) -o $@

bin/hybrid_heuristic_tgt: $(OBJ_TGT_FILES)
	$(LINK) $(OBJ_TGT_FILES) $(LIBS) -o $@

bin/hybrid_heuristic_prof: $(OBJ_PROF_FILES)
	$(LINK_PROF) $(OBJ_PROF_FILES) $(LIBS) -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

dep/%.d: src/%.cpp include/*.h
	$(DEP) $(DEPFLAGS) -MM $(patsubst dep/%.d,src/%.cpp,$@) -MT $(patsubst dep/%.d,obj/opt/%.o,$@) -MT $(patsubst dep/%.d,obj/dbg/%.o,$@) -MT $(patsubst dep/%.d,obj/tgt/%.o,$@) -MT $(patsubst dep/%.d,obj/prof/%.o,$@) $(INCLUDE_DIRS) > $@

obj/dbg/%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS_DBG) $< -o $@

obj/opt/%.o: src/%.cpp
	$(CXX) -DNDEBUG -c $(CXXFLAGS_OPT) $< -o $@

obj/tgt/%.o: src/%.cpp
	$(CXX) -DNDEBUG -c $(CXXFLAGS_TGT) $< -o $@

obj/prof/%.o: src/%.cpp
	$(CXX) -DNDEBUG -c $(CXXFLAGS_PROF) $< -o $@


clean:
	rm -f bin/*
	rm -f dep/*.d
	rm -f obj/opt/*.o
	rm -f obj/dbg/*.o
	rm -f obj/tgt/*.o
	rm -f obj/prof/*.o

.PHONY: clean
