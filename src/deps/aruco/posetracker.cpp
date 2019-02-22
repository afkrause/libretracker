#include <opencv2/calib3d/calib3d.hpp>
#include "posetracker.h"
#if CV_VERSION_MAJOR == 2
#include "levmarq.h"
#endif
#include "ippe.h"

namespace aruco{

cv::Mat impl__aruco_getRTMatrix(const cv::Mat &_rvec,const cv::Mat &_tvec){
    if (_rvec.empty())return cv::Mat();
        cv::Mat Matrix=cv::Mat::eye ( 4,4,CV_32FC1 );
        cv::Mat R33=cv::Mat ( Matrix,cv::Rect ( 0,0,3,3 ) );
        cv::Rodrigues ( _rvec,R33 );
        for ( int i=0; i<3; i++ ) Matrix.at<float> ( i,3 ) =_tvec.ptr<float> ( 0 ) [i];
        return Matrix;

}
void impl__aruco_getRTfromMatrix44 ( const cv::Mat &M,  cv::Mat &R,cv::Mat &T ) {

    assert ( M.cols==M.rows && M.cols==4 );
    assert ( M.type() ==CV_32F || M.type() ==CV_64F );
//extract the rotation part
    cv::Mat r33=cv::Mat ( M,cv::Rect ( 0,0,3,3 ) );
    cv::SVD svd ( r33 );
    cv::Mat Rpure=svd.u*svd.vt;
    cv::Rodrigues ( Rpure,R );
    T.create ( 1,3,M.type() );
    if ( M.type() ==CV_32F )
        for ( int i=0; i<3; i++ )
            T.ptr<float> ( 0 ) [i]=M.at<float> ( i,3 );
    else
        for ( int i=0; i<3; i++ )
            T.ptr<double> ( 0 ) [i]=M.at<double> ( i,3 );
}
#if  CV_VERSION_MAJOR == 2

template<typename T>
double __aruco_solve_pnp(const std::vector<cv::Point3f> & p3d,const std::vector<cv::Point2f> & p2d,const cv::Mat &cam_matrix,const cv::Mat &dist,cv::Mat &r_io,cv::Mat &t_io){

    assert(r_io.type()==CV_32F);
    assert(t_io.type()==CV_32F);
    assert(t_io.total()==r_io.total());
    assert(t_io.total()==3);
    auto toSol=[](const cv::Mat &r,const cv::Mat &t){
          typename LevMarq<T>::eVector sol(6);
        for(int i=0;i<3;i++){
            sol(i)=r.ptr<float>(0)[i];
            sol(i+3)=t.ptr<float>(0)[i];
        }
        return sol;
    };
    auto fromSol=[](const typename LevMarq<T>::eVector &sol,cv::Mat &r,cv::Mat &t){
        r.create(1,3,CV_32F);
        t.create(1,3,CV_32F);
        for(int i=0;i<3;i++){
            r.ptr<float>(0)[i]=sol(i);
            t.ptr<float>(0)[i]=sol(i+3);
        }
    };

    cv::Mat Jacb;
    auto err_f= [&](const  typename LevMarq<T>::eVector &sol,typename LevMarq<T>::eVector &err){
        std::vector<cv::Point2f> p2d_rej;
        cv::Mat r,t;
        fromSol(sol,r,t);
        cv::projectPoints(p3d,r,t,cam_matrix,dist,p2d_rej,Jacb);
        err.resize(p3d.size()*2);
        int err_idx=0;
        for(size_t i=0;i<p3d.size();i++){
            err(err_idx++)=p2d_rej[i].x-p2d[i].x;
            err(err_idx++)=p2d_rej[i].y-p2d[i].y;
        }
    };
    auto jac_f=[&](const  typename LevMarq<T>::eVector &sol,Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> &J){
        (void)(sol);
      J.resize(p3d.size()*2,6);
      for(size_t i=0;i<p3d.size()*2;i++){
          double *jacb=Jacb.ptr<double>(i);
          for(int j=0;j<6;j++) J(i,j)=jacb[j];
      }
    };

    LevMarq<T> solver;
    solver.setParams(100,0.01,0.01);
  //  solver.verbose()=true;
    typename LevMarq<T>::eVector sol=toSol(r_io,t_io);
    auto err=solver.solve(sol,err_f,jac_f);

    fromSol(sol,r_io,t_io);
    return err;

}

double __aruco_solve_pnp(const std::vector<cv::Point3f> & p3d,const std::vector<cv::Point2f> & p2d,const cv::Mat &cam_matrix,const cv::Mat &dist,cv::Mat &r_io,cv::Mat &t_io){
#ifdef DOUBLE_PRECISION_PNP
    return __aruco_solve_pnp<double>(p3d,p2d,cam_matrix,dist,r_io,t_io);
#else
    return __aruco_solve_pnp<float>(p3d,p2d,cam_matrix,dist,r_io,t_io);
#endif
}

#endif
bool MarkerPoseTracker::estimatePose(  Marker &m,const   CameraParameters &_cam_params,float _msize,float minerrorRatio){


    if (_rvec.empty()){//if no previous data, use from scratch
        cv::Mat rv,tv;
        auto solutions=IPPE::solvePnP_(Marker::get3DPoints(_msize),m,_cam_params.CameraMatrix,_cam_params.Distorsion);
        double errorRatio=solutions[1].second/solutions[0].second;
        if (errorRatio<minerrorRatio) return false;//is te error ratio big enough
        cv::solvePnP(Marker::get3DPoints(_msize),m,_cam_params.CameraMatrix,_cam_params.Distorsion,rv,tv);
         rv.convertTo(_rvec,CV_32F);
        tv.convertTo(_tvec,CV_32F);
        impl__aruco_getRTfromMatrix44(solutions[0].first,rv,tv);
     }
    else{
#if  CV_VERSION_MAJOR >= 3
        cv::solvePnP(Marker::get3DPoints(_msize),m,_cam_params.CameraMatrix,_cam_params.Distorsion,_rvec,_tvec,true);
#else //solve Pnp does not work properly in OpenCV2
        __aruco_solve_pnp(Marker::get3DPoints(_msize),m,_cam_params.CameraMatrix,_cam_params.Distorsion,_rvec,_tvec);
#endif

    }

    _rvec.copyTo(m.Rvec);
    _tvec.copyTo(m.Tvec);
    m.ssize=_msize;
    return true;
}




MarkerMapPoseTracker::MarkerMapPoseTracker(){
    _isValid=false;
}

void MarkerMapPoseTracker::setParams(const  CameraParameters &cam_params,const MarkerMap &msconf, float markerSize)throw(cv::Exception)
{

    _msconf=msconf;
    _cam_params=cam_params;
    if (!cam_params.isValid())
        throw cv::Exception(9001, "Invalid camera parameters", "MarkerMapPoseTracker::setParams", __FILE__, __LINE__);
    if (_msconf.mInfoType==MarkerMap::PIX && markerSize<=0)
        throw cv::Exception(9001, "You should indicate the markersize sice the MarkerMap is in pixels", "MarkerMapPoseTracker::setParams", __FILE__, __LINE__);
    if (_msconf.mInfoType==MarkerMap::NONE)
        throw cv::Exception(9001, "Invlaid MarkerMap", "MarkerMapPoseTracker::setParams", __FILE__, __LINE__);
    if (_msconf.mInfoType==MarkerMap::PIX)
        _msconf=_msconf.convertToMeters(markerSize);

    _isValid=true;

    //create a map for fast access to elements
    _map_mm.clear();
    for(auto m:msconf)
        _map_mm.insert(make_pair(m.id,m));
}

bool MarkerMapPoseTracker::estimatePose(const  vector<Marker> &v_m){


    vector<cv::Point2f> p2d;
    vector<cv::Point3f> p3d;
    for(auto marker:v_m){
        if ( _map_mm.find(marker.id)!=_map_mm.end()){//is the marker part of the map?
            for(auto p:marker)  p2d.push_back(p);
            for(auto p:_map_mm[marker.id])  p3d.push_back(p);
        }
    }

    if (p2d.size()==0){//no points in the vector
        _rvec=cv::Mat();_tvec=cv::Mat();return false;
    }
    else{
        if(_rvec.empty()){//requires ransac since past pose is unknown
            cv::Mat rv,tv;
            cv::solvePnPRansac(p3d,p2d,_cam_params.CameraMatrix,_cam_params.Distorsion,rv,tv);


            assert(tv.type()==CV_64F);
            if (_rvec.rows==1) {
                rv.convertTo(_rvec,CV_32F);
                tv.convertTo(_tvec,CV_32F);
            }
            else{

                _rvec.create(1,3,CV_32F);
                _tvec.create(1,3,CV_32F);
                for(int i=0;i<3;i++){
                    _rvec.ptr<float>(0)[i]=rv.at<double>(i,0);
                    _tvec.ptr<float>(0)[i]=tv.at<double>(i,0);
                }
            }
        }

#if  CV_VERSION_MAJOR >= 3
        cv::solvePnP(p3d,p2d,_cam_params.CameraMatrix,_cam_params.Distorsion,_rvec,_tvec,true);

#else //SolvePnP does not work properly in opencv 2
        __aruco_solve_pnp(p3d,p2d,_cam_params.CameraMatrix,_cam_params.Distorsion,_rvec,_tvec);

#endif

        return true;
    }
}

cv::Mat MarkerMapPoseTracker::getRTMatrix (  ) const {
    return impl__aruco_getRTMatrix(_rvec,_tvec);
}


cv::Mat MarkerPoseTracker::getRTMatrix (  ) const {
    return impl__aruco_getRTMatrix(_rvec,_tvec);
}
}
