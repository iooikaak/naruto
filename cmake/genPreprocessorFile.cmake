#生成符合 gcc 预编译时的头文件路径语法：-I dir1 -I dir2 ...，将其存储到 var 变量中
function(source_header_dir var)
    #将变量值存储
    string(APPEND gcc_formate_header ${${var}})
    #遍历参数
    foreach(_header ${ARGN})
        #获取绝对路径
        get_filename_component(full_path ${_header} ABSOLUTE)
        string(APPEND gcc_formate_header "-I ${full_path} ")
    endforeach()
    #对 var 变量进行赋值，需要使用 PARENT_SCOPE 参数
    set(${var} ${gcc_formate_header} PARENT_SCOPE)
endfunction()

#生成预编译文件，参数 headers 中包含的是上个函数生成的头文件
function(gen_preprocessor_file headers)
    #sh脚本文件的名字
    set(shFileName "${CMAKE_SOURCE_DIR}/cmake/__pre.sh")
    #保存预编译文件的目录
    set(preFilesDirName "pre_files")
    #创建预编译文件目录
    execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${preFilesDirName})
    #遍历源码文件，生成预编译文件
    foreach(_file ${ARGN})
        #获取源码文件的绝对路径
        get_filename_component(file_full_path ${_file} ABSOLUTE)
        #获取源码文件的文件名(不包含后缀)，比如源码文件为 a/b/test.c，文件名为 test
        get_filename_component(only_file_name ${_file} NAME_WE)
        #调用脚本，生成预编译文件
        execute_process(COMMAND sh ${shFileName} ${file_full_path} ${headers} ${CMAKE_CURRENT_BINARY_DIR}/${preFilesDirName}/${only_file_name}.i)
    endforeach()
endfunction()