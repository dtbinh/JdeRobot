/*
 *  Copyright (C) 1997-2013 JDERobot Developers Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors : Borja Menéndez <borjamonserrano@gmail.com>
 *
 */

#include "poserightknee.h"

#define RADTODEG 57.29582790

namespace gazebo {
    GZ_REGISTER_MODEL_PLUGIN(PoseRightKnee)

    PoseRightKnee::PoseRightKnee () {
        pthread_mutex_init(&this->mutex_rightkneeencoders, NULL);
        pthread_mutex_init(&this->mutex_rightkneemotors, NULL);
        this->count = 0;
        this->cycle = 50;
        this->cfgfile_rightknee = std::string("--Ice.Config=poserightknee.cfg");
        
        this->rightknee.motorsparams.maxPan = 1.57;
        this->rightknee.motorsparams.minPan = -1.57;          
        this->rightknee.motorsparams.maxTilt = 0.5;
        this->rightknee.motorsparams.minTilt = -0.5;

        std::cout << "Constructor PoseRightKnee" << std::endl;
    }

    void PoseRightKnee::Load ( physics::ModelPtr _model, sdf::ElementPtr _sdf ) {
        // LOAD CAMERA LEFT
        if (!_sdf->HasElement("joint_pose3dencodersrightknee_pitch"))
            gzerr << "pose3dencodersrightknee plugin missing <joint_pose3dencodersrightknee_pitch> element\n";

        this->rightknee.joint_tilt = _model->GetJoint("right_knee");

        if (!this->rightknee.joint_tilt)
            gzerr << "Unable to find joint_pose3dencodersrightknee_pitch["
                << _sdf->GetElement("joint_pose3dencodersrightknee_pitch")->GetValueString() << "]\n"; 
                
        this->rightknee.link_tilt = _model->GetLink("right_shin");

        //LOAD TORQUE        
        if (_sdf->HasElement("torque"))
            this->stiffness = _sdf->GetElement("torque")->GetValueDouble();
        else {
            gzwarn << "No torque value set for the DiffDrive plugin.\n";
            this->stiffness = 5.0;
        }

        //LOAD POSE3DMOTORS
        this->updateConnection = event::Events::ConnectWorldUpdateBegin(
                                    boost::bind(&PoseRightKnee::OnUpdate, this));
    }

    void PoseRightKnee::Init () {}

    void PoseRightKnee::OnUpdate () {
        long totalb, totala, diff;
        struct timeval a, b;
        
        gettimeofday(&a, NULL);
        totala = a.tv_sec * 1000000 + a.tv_usec;

        if (this->count == 0) {
            this->count++;
            pthread_t thr_ice;
            pthread_create(&thr_ice, NULL, &thread_RightKneeICE, (void*) this);
        }

        //          ----------ENCODERS----------
        //GET pose3dencoders data from the right knee (PAN&TILT)  
        this->rightknee.encoders.tilt = this->rightknee.link_tilt->GetRelativePose().rot.GetAsEuler().x;

        //          ----------MOTORS----------
        if (this->rightknee.motorsdata.tilt >= 0) {
            if (this->rightknee.encoders.tilt < this->rightknee.motorsdata.tilt) {
                this->rightknee.joint_tilt->SetVelocity(0, -0.1);
                this->rightknee.joint_tilt->SetMaxForce(0, this->stiffness);
            } else {
                this->rightknee.joint_tilt->SetVelocity(0, 0.1);
                this->rightknee.joint_tilt->SetMaxForce(0, this->stiffness);
            }
        } else {
            if (this->rightknee.encoders.tilt > this->rightknee.motorsdata.tilt) {
                this->rightknee.joint_tilt->SetVelocity(0, 0.1);
                this->rightknee.joint_tilt->SetMaxForce(0, this->stiffness);
            } else {
                this->rightknee.joint_tilt->SetVelocity(0, -0.1);
                this->rightknee.joint_tilt->SetMaxForce(0, this->stiffness);
            }
        }

        gettimeofday(&b, NULL);
        totalb = b.tv_sec * 1000000 + b.tv_usec;

        diff = (totalb - totala) / 1000;
        diff = cycle - diff;

        if (diff < 10)
            diff = 10;

        //usleep(diff*1000);
        sleep(diff / 1000);
    }
    
    class Pose3DEncoders : virtual public jderobot::Pose3DEncoders {
    public:

        Pose3DEncoders ( gazebo::PoseRightKnee* pose ) : pose3DEncodersData ( new jderobot::Pose3DEncodersData() ) {
            this->pose = pose;
        }

        virtual ~Pose3DEncoders () {}

        virtual jderobot::Pose3DEncodersDataPtr getPose3DEncodersData ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightkneeencoders);
            
            pose3DEncodersData->x = pose->rightknee.encoders.x;
            pose3DEncodersData->y = pose->rightknee.encoders.y;
            pose3DEncodersData->z = pose->rightknee.encoders.z;
            pose3DEncodersData->pan = pose->rightknee.encoders.pan;
            pose3DEncodersData->tilt = pose->rightknee.encoders.tilt;
            pose3DEncodersData->roll = pose->rightknee.encoders.roll;
            pose3DEncodersData->clock = pose->rightknee.encoders.clock;
            pose3DEncodersData->maxPan = pose->rightknee.encoders.maxPan;
            pose3DEncodersData->minPan = pose->rightknee.encoders.minPan;
            pose3DEncodersData->maxTilt = pose->rightknee.encoders.maxTilt;
            pose3DEncodersData->minTilt = pose->rightknee.encoders.minTilt;
            
            pthread_mutex_unlock(&pose->mutex_rightkneeencoders);

            return pose3DEncodersData;
        }

        gazebo::PoseRightKnee* pose;

    private:
        jderobot::Pose3DEncodersDataPtr pose3DEncodersData;
    };

    class Pose3DMotors : virtual public jderobot::Pose3DMotors {
    public:

        Pose3DMotors (gazebo::PoseRightKnee* pose) : pose3DMotorsData ( new jderobot::Pose3DMotorsData() ) {
            this->pose = pose;
        }

        virtual ~Pose3DMotors() {}

        virtual jderobot::Pose3DMotorsDataPtr getPose3DMotorsData ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightkneemotors);
            
            pose3DMotorsData->x = pose->rightknee.motorsdata.x;
            pose3DMotorsData->y = pose->rightknee.motorsdata.y;
            pose3DMotorsData->z = pose->rightknee.motorsdata.z;
            pose3DMotorsData->pan = pose->rightknee.motorsdata.pan;
            pose3DMotorsData->tilt = pose->rightknee.motorsdata.tilt;
            pose3DMotorsData->roll = pose->rightknee.motorsdata.roll;
            pose3DMotorsData->panSpeed = pose->rightknee.motorsdata.panSpeed;
            pose3DMotorsData->tiltSpeed = pose->rightknee.motorsdata.tiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightkneemotors);

            return pose3DMotorsData;
        }

        virtual jderobot::Pose3DMotorsParamsPtr getPose3DMotorsParams ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightkneemotors);
            
            pose3DMotorsParams->maxPan = pose->rightknee.motorsparams.maxPan;
            pose3DMotorsParams->minPan = pose->rightknee.motorsparams.minPan;
            pose3DMotorsParams->maxTilt = pose->rightknee.motorsparams.maxTilt;
            pose3DMotorsParams->minTilt = pose->rightknee.motorsparams.minTilt;
            pose3DMotorsParams->maxPanSpeed = pose->rightknee.motorsparams.maxPanSpeed;
            pose3DMotorsParams->maxTiltSpeed = pose->rightknee.motorsparams.maxTiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightkneemotors);
            
            return pose3DMotorsParams;
        }

        virtual Ice::Int setPose3DMotorsData ( const jderobot::Pose3DMotorsDataPtr & data, const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightkneemotors);
            
            pose->rightknee.motorsdata.x = data->x;
            pose->rightknee.motorsdata.y = data->y;
            pose->rightknee.motorsdata.z = data->z;
            pose->rightknee.motorsdata.pan = data->pan;
            pose->rightknee.motorsdata.tilt = data->tilt;
            pose->rightknee.motorsdata.roll = data->roll;
            pose->rightknee.motorsdata.panSpeed = data->panSpeed;
            pose->rightknee.motorsdata.tiltSpeed = data->tiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightkneemotors);
        }

        gazebo::PoseRightKnee* pose;

    private:
        jderobot::Pose3DMotorsDataPtr pose3DMotorsData;
        jderobot::Pose3DMotorsParamsPtr pose3DMotorsParams;
    };

    void* thread_RightKneeICE ( void* v ) {

        gazebo::PoseRightKnee* rightknee = (gazebo::PoseRightKnee*)v;
        char* name = (char*) rightknee->cfgfile_rightknee.c_str();
        Ice::CommunicatorPtr ic;
        int argc = 1;
        Ice::PropertiesPtr prop;
        char* argv[] = {name};

        try {
            ic = Ice::initialize(argc, argv);

            prop = ic->getProperties();
            std::string EndpointsEncoders = prop->getProperty("PoseRightKneeEncoders.Endpoints");
            std::cout << "PoseRightKneeEncoders Endpoints > " << EndpointsEncoders << std::endl;
            std::string EndpointsMotors = prop->getProperty("PoseRightKneeMotors.Endpoints");
            std::cout << "PoseRightKneeMotors Endpoints > " << EndpointsMotors << std::endl;

            Ice::ObjectAdapterPtr AdapterEncoders =
                    ic->createObjectAdapterWithEndpoints("AdapterRightKneeEncoders", EndpointsEncoders);
            Ice::ObjectAdapterPtr AdapterMotors =
                    ic->createObjectAdapterWithEndpoints("AdapterRightKneeMotors", EndpointsMotors);

            Ice::ObjectPtr encoders = new Pose3DEncoders(rightknee);
            Ice::ObjectPtr motors = new Pose3DMotors(rightknee);

            AdapterEncoders->add(encoders, ic->stringToIdentity("RightKneeEncoders"));
            AdapterMotors->add(motors, ic->stringToIdentity("RightKneeMotors"));

            AdapterEncoders->activate();
            AdapterMotors->activate();

            ic->waitForShutdown();
        } catch (const Ice::Exception& e) {
            std::cerr << e << std::endl;
        } catch (const char* msg) {
            std::cerr << msg << std::endl;
        }
        if (ic) {
            try {
                ic->destroy();
            } catch (const Ice::Exception& e) {
                std::cerr << e << std::endl;
            }
        }

    }

}
