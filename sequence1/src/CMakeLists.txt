project(pretr)

#

add_executable (pretr pretr_main.cpp)

target_link_libraries (pretr ${BOOSTLIBS}  X11 pthread ${UMFLIBS})

#

#add_executable (pretr_test pretr_test.cpp) 

#target_link_libraries (pretr_test ${BOOSTLIBS} X11 pthread ${UMFLIBS})

#
add_executable (vis_trlet vis_trlet.cpp) 

target_link_libraries (vis_trlet ${BOOSTLIBS} X11 pthread ${UMFLIBS})

#

add_executable (filter_trlet filter_trlet_main.cpp) 

target_link_libraries (filter_trlet ${BOOSTLIBS} X11 pthread ${UMFLIBS})


##################################################################
# appearance only model
#
add_executable (linprog_app linprog_app_main.cpp)

target_link_libraries (linprog_app  ${BOOSTLIBS} X11 pthread ${UMFLIBS})

#
#add_executable (linprog_app_test linprog_app_test.cpp)

#target_link_libraries (linprog_app_test  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


#
add_executable (vis_linprog_app vis_linprog_app.cpp)

target_link_libraries (vis_linprog_app  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


##################################################################
# appearance +planning model
#
add_executable (linprog_plan linprog_plan_main.cpp)

target_link_libraries (linprog_plan  ${BOOSTLIBS} X11 pthread ${UMFLIBS})

#
#add_executable (linprog_plan_test linprog_plan_test.cpp)

#target_link_libraries (linprog_plan_test  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


#
add_executable (vis_linprog_plan vis_linprog_plan.cpp)

target_link_libraries (vis_linprog_plan  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


##################################################################
# appearance +alone model
#
add_executable (linprog_alone linprog_alone_main.cpp)

target_link_libraries (linprog_alone  ${BOOSTLIBS} X11 pthread ${UMFLIBS})

#
add_executable (vis_linprog_alone vis_linprog_alone.cpp)

target_link_libraries (vis_linprog_alone  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


#
add_executable (generate_video generate_video.cpp)

target_link_libraries (generate_video  ${BOOSTLIBS} X11 pthread ${UMFLIBS})


###

add_executable (munkres_test munkres_main.cpp)

#target_link_libraries (munkres_test  ${BOOSTLIBS} X11 pthread ${UMFLIBS})

###

add_executable (perf_eval perf_eval_main.cpp)
target_link_libraries (perf_eval  ${BOOSTLIBS} X11 pthread ${UMFLIBS})

add_executable (vis_gtrst_pair vis_gtrst_pair.cpp)
target_link_libraries (vis_gtrst_pair  ${BOOSTLIBS} X11 pthread ${UMFLIBS})
