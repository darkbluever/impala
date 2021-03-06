#  Copyright (c) 2012 Cloudera, Inc. All rights reserved.

# Source this file from the $IMPALA_HOME directory to
# setup your environment. If $IMPALA_HOME is undefined
# this script will set it to the current working directory.

if [ -z $IMPALA_HOME ]; then
    this=${0/-/} # login-shells often have leading '-' chars
    shell_exec=`basename $SHELL`
    if [ "$this" = "$shell_exec" ]; then
        # Assume we're already in IMPALA_HOME
        interactive=1
        export IMPALA_HOME=`pwd`
    else
        interactive=0
        while [ -h "$this" ]; do
            ls=`ls -ld "$this"`
            link=`expr "$ls" : '.*-> \(.*\)$'`
            if expr "$link" : '.*/.*' > /dev/null; then
                this="$link"
            else
                this=`dirname "$this"`/"$link"
            fi
        done
        
        # convert relative path to absolute path
        bin=`dirname "$this"`
        script=`basename "$this"`
        bin=`cd "$bin"; pwd`
        this="$bin/$script"
        
        export IMPALA_HOME=`dirname "$bin"`
    fi
fi


export IMPALA_FE_DIR=$IMPALA_HOME/fe
export IMPALA_BE_DIR=$IMPALA_HOME/be
export IMPALA_WORKLOAD_DIR=$IMPALA_HOME/testdata/workloads
export IMPALA_DATASET_DIR=$IMPALA_HOME/testdata/datasets
export IMPALA_COMMON_DIR=$IMPALA_HOME/common
export PATH=$IMPALA_HOME/bin:$PATH

export HADOOP_HOME=$IMPALA_HOME/thirdparty/hadoop-2.0.0-cdh4.1.0/
export HADOOP_CONF_DIR=$IMPALA_FE_DIR/src/test/resources
export MINI_DFS_BASE_DATA_DIR=$HADOOP_HOME/build/test/data
export PATH=$HADOOP_HOME/bin:$PATH

export HIVE_HOME=$IMPALA_HOME/thirdparty/hive-0.9.0-cdh4.1.0/
export PATH=$HIVE_HOME/bin:$PATH
export HIVE_CONF_DIR=$IMPALA_FE_DIR/src/test/resources
export HIVE_AUX_JARS_PATH=$IMPALA_FE_DIR/target

export HBASE_HOME=$IMPALA_HOME/thirdparty/hbase-0.92.1-cdh4.1.0/
export PATH=$HBASE_HOME/bin:$PATH
export HBASE_CONF_DIR=$HIVE_CONF_DIR

# set the python path for test modules and beeswax
PYTHONPATH=$IMPALA_HOME:$IMPALA_HOME/shell/gen-py:$HIVE_HOME/lib/py
export PYTHONPATH=$PYTHONPATH:$IMPALA_HOME/thirdparty/python-thrift-0.7.0

# These arguments are, despite the name, passed to every JVM created
# by an impalad. So they must point to the location of
# libbackend.so. 
LIBHDFS_OPTS="-Djava.library.path=${HADOOP_HOME}/lib/native/"
# READER BEWARE: This always points to the debug build. 
# TODO: Consider having cmake scripts change this value depending on
# the build type.
export LIBHDFS_OPTS="${LIBHDFS_OPTS}:${IMPALA_HOME}/be/build/debug/service"

export ARTISTIC_STYLE_OPTIONS=$IMPALA_BE_DIR/.astylerc

export JAVA_LIBRARY_PATH=${IMPALA_HOME}/thirdparty/snappy-1.0.5/build/lib

# So that the frontend tests and PlanService can pick up libbackend.so
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${IMPALA_HOME}/be/build/debug/service"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${IMPALA_HOME}/thirdparty/snappy-1.0.5/build/lib"

CLASSPATH=$IMPALA_FE_DIR/target/dependency:$CLASSPATH
CLASSPATH=$IMPALA_FE_DIR/target/classes:$CLASSPATH
CLASSPATH=$IMPALA_FE_DIR/src/test/resources:$CLASSPATH

export CLASSPATH

echo "IMPALA_HOME            = $IMPALA_HOME"
echo "HADOOP_HOME            = $HADOOP_HOME"
echo "HADOOP_CONF_DIR        = $HADOOP_CONF_DIR"
echo "MINI_DFS_BASE_DATA_DIR = $MINI_DFS_BASE_DATA_DIR"
echo "HIVE_HOME              = $HIVE_HOME"
echo "HIVE_CONF_DIR          = $HIVE_CONF_DIR"
echo "HBASE_HOME             = $HBASE_HOME"
echo "HBASE_CONF_DIR         = $HBASE_CONF_DIR"
echo "PYTHONPATH"            = $PYTHONPATH
echo "CLASSPATH              = $CLASSPATH"
