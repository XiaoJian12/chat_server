# 定义源文件列表
aux_source_directory(. SRC_LIST)

# 生成可执行文件
add_executable(ChatClient ${SRC_LIST})

# 依赖库
target_link_libraries(ChatClient pthread)