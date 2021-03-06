//
//  poolingLayer.h
//  LightCTR
//
//  Created by SongKuangshi on 2017/10/24.
//  Copyright © 2017年 SongKuangshi. All rights reserved.
//

#ifndef poolingLayer_h
#define poolingLayer_h

#include <vector>
#include "../../util/matrix.h"
#include "layer_abst.h"

struct Pool_Config {
    size_t size;
};
// Pooling or Maxout
// TODO K-Max Pooling
template <typename ActivationFunction>
class Max_Pooling_Layer : public Layer_Base {
public:
    Max_Pooling_Layer(Layer_Base* _prevLayer, size_t _dimension, Pool_Config _config):
    Layer_Base(_prevLayer, _dimension, _dimension), config(_config) {
        this->activeFun = new ActivationFunction();
        assert(this->input_dimension == this->output_dimension);
        
        printf("Pooling Layer\n");
    }
    Max_Pooling_Layer() = delete;
    
    ~Max_Pooling_Layer() {
    }
    
    vector<float>& forward(const vector<Matrix*>& prevLOutput) {
        assert(prevLOutput.size() == this->input_dimension);
        
        // init ThreadLocal var
        MatrixArr& output_act = *tl_output_act;
        output_act.arr.resize(this->output_dimension);
        MatrixArr& input_delta = *tl_input_delta;
        input_delta.arr.resize(this->input_dimension);
        
        // do Max pooling
        FOR(feamid, this->input_dimension) {
            Matrix* mat = prevLOutput[feamid];
            
            assert(mat->x_len >= config.size && mat->y_len >= config.size);
            
            if (input_delta.arr[feamid] == NULL) {
                output_act.arr[feamid] = new Matrix((mat->x_len - config.size) / config.size + 1,
                                                (mat->y_len - config.size) / config.size + 1);
                input_delta.arr[feamid] = new Matrix(mat->x_len, mat->y_len);
            }
            
            auto cur_out = output_act.arr[feamid];
            cur_out->zeroInit();
            auto cur_in = input_delta.arr[feamid];
            cur_in->zeroInit();
            for (size_t i = 0; i < mat->x_len - config.size + 1; i+= config.size) {
                for (size_t j = 0; j < mat->y_len - config.size + 1; j+=config.size) {
                    float MaxV = *mat->getEle(i, j);
                    size_t mx = i, my = j;
                    for (size_t x = i; x < i + config.size; x++) {
                        for (size_t y = j; y < j + config.size; y++) {
                            if (MaxV < *mat->getEle(x, y)) {
                                MaxV = *mat->getEle(x, y);
                                mx = x, my = y;
                            }
                        }
                    }
                    *cur_out->getEle(i / config.size, j / config.size) = MaxV;
                    *cur_in->getEle(mx, my) = 1;
                }
            }
        }
        return this->nextLayer->forward(output_act.arr);
    }
    
    void backward(const vector<Matrix*>& outputDelta) {
        assert(outputDelta.size() == this->output_dimension);
        
        MatrixArr& input_delta = *tl_input_delta;
        
        // Unpooling
        FOR(fid, this->input_dimension) {
            Matrix* mat = input_delta.arr[fid];
            for (size_t i = 0; i < mat->x_len - config.size + 1; i+= config.size) {
                for (size_t j = 0; j < mat->y_len - config.size + 1; j+=config.size) {
                    // loop pooling size
                    for (size_t x = i; x < i + config.size; x++) {
                        for (size_t y = j; y < j + config.size; y++) {
                            if (*mat->getEle(x, y) > 0) {
                                *mat->getEle(x, y) = *outputDelta[fid]->getEle(i / config.size, j / config.size);
                            }
                        }
                    }
                }
            }
        }
        return this->prevLayer->backward(input_delta.arr);
    }
    
    const vector<Matrix*>& output() {
        MatrixArr& output_act = *tl_output_act;
        return output_act.arr;
    }
    
private:
    Pool_Config config;
    ThreadLocal<MatrixArr> tl_output_act;
    ThreadLocal<MatrixArr> tl_input_delta; // mask of max position
};

#endif /* poolingLayer_h */
