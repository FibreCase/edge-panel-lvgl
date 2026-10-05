#include "platform/linux/coordinates.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>
namespace {
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
}
int main() {
    using namespace panel;
    try {
        std::vector<uint32_t> source(logical_width*logical_height);
        for(size_t i=0;i<source.size();++i) source[i]=static_cast<uint32_t>(i);
        const uint32_t pitch=(physical_width+16)*4;
        std::vector<uint32_t> destination(pitch/4*physical_height,0xDEADBEEF);
        for(int rotation:{90,270}) {
            rotate_frame(source.data(),reinterpret_cast<uint8_t*>(destination.data()),pitch,rotation);
            for(int y=0;y<logical_height;++y) for(int x=0;x<logical_width;++x) {
                Point physical=to_physical({x,y},rotation);
                check(physical.x>=0&&physical.x<physical_width&&physical.y>=0&&physical.y<physical_height,"physical bounds");
                Point logical=to_logical(physical,rotation);
                check(logical.x==x&&logical.y==y,"rotation inverse");
                check(destination[physical.y*pitch/4+physical.x]==source[y*logical_width+x],"scanout pixel rotation");
            }
            for(int y=0;y<physical_height;++y) for(int x=physical_width;x<static_cast<int>(pitch/4);++x)
                check(destination[y*pitch/4+x]==0xDEADBEEF,"pitch padding overwritten");
        }
        Point clockwise=to_physical({0,0},90),counter=to_physical({0,0},270);
        check(clockwise.x==719&&clockwise.y==0,"90-degree direction");
        check(counter.x==0&&counter.y==1279,"270-degree direction");
        TouchTransform touch;touch.xmin=100;touch.xmax=4095;touch.ymin=-50;touch.ymax=2000;
        for(int rotation:{90,270}) for(bool swap:{false,true}) for(bool ix:{false,true}) for(bool iy:{false,true}) {
            touch.rotation=rotation;touch.swap=swap;touch.invert_x=ix;touch.invert_y=iy;
            for(int x:{touch.xmin,touch.xmax}) for(int y:{touch.ymin,touch.ymax}) {
                auto point=touch.map(x,y);auto physical=to_physical(point,rotation);
                int ex=(swap?y==touch.ymax:x==touch.xmax)?719:0;
                int ey=(swap?x==touch.xmax:y==touch.ymax)?1279:0;
                if(ix) ex=719-ex;if(iy) ey=1279-ey;
                check(physical.x==ex&&physical.y==ey,"touch calibration and rotation");
            }
        }
        touch={};touch.xmax=4095;touch.ymax=4095;touch.logical=true;
        auto point=touch.map(4095,4095);
        check(point.x==1279&&point.y==719,"logical touch must not rotate twice");
        check(normalize_axis(-1,0,4095,720)==0,"axis clamp low");
        check(normalize_axis(5000,0,4095,720)==719,"axis clamp high");
        bool rejected=false;
        try { to_logical({0,0},0); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"reject unsupported rotation");
        rejected=false;
        try { normalize_axis(0,0,0,720); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"reject invalid axis range");
        std::cout<<"Display rotation, stride and touch mapping passed\n";return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
