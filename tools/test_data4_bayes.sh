#!/bin/sh

rm -f iris-test.ds

# creates training dataset for nntool

./dstool -create iris-test.ds
./dstool -create:4:input iris-test.ds
./dstool -create:1:output iris-test.ds
./dstool -list iris-test.ds
./dstool -import:0 iris-test.ds iris.in
./dstool -import:1 iris-test.ds iris.out
./dstool -padd:0:meanvar iris-test.ds
./dstool -padd:0:pca iris-test.ds
./dstool -padd:1:meanvar iris-test.ds

./dstool -list iris-test.ds

# uses nntool trying to learn from dataset

./nntool -v --threads 1 --adaptive --samples 1000  iris-test.ds 4-10-1 iris-nn.cfg lbfgs
./nntool -v --threads 1 --adaptive --load --samples 20000 iris-test.ds 4-10-1 iris-nn.cfg bayes

##################################################
# testing

./nntool -v iris-test.ds 4-10-1 iris-nn.cfg use

##################################################
# predicting [stores results to dataset]

cp -f iris-test.ds iris-pred.ds
./dstool -clear:1 iris-pred.ds
# ./dstool -remove:1 wine-pred.ds

./nntool -v iris-pred.ds 4-10-1 iris-nn.cfg use

./dstool -list iris-test.ds
./dstool -list iris-pred.ds

./dstool -print:1 iris-pred.ds
tail iris.out

echo "Bayesian inference with deep network structure DO NOT converge"



