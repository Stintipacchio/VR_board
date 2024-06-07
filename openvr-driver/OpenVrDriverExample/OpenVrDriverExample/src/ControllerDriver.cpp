#include <ControllerDriver.h>
#include <openvr_driver.h>
#include <Windows.h>
#include <Xinput.h>

EVRInitError ControllerDriver::Activate(uint32_t unObjectId)
{
	driverId = unObjectId; //unique ID for your driver

	PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(driverId); //this gets a container object where you store all the information about your driver

	VRProperties()->SetStringProperty(props, Prop_InputProfilePath_String, "{example}/input/controller_profile.json"); //tell OpenVR where to get your driver's Input Profile
	VRProperties()->SetInt32Property(props, Prop_ControllerRoleHint_Int32, ETrackedControllerRole::TrackedControllerRole_Treadmill); //tells OpenVR what kind of device this is
	VRDriverInput()->CreateScalarComponent(props, "/input/joystick/y", &joystickYHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //sets up handler you'll use to send joystick commands to OpenVR with, in the Y direction (forward/backward)
	//VRDriverInput()->CreateScalarComponent(props, "/input/trackpad/y", &trackpadYHandle, EVRScalarType::VRScalarType_Absolute,
	//	EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //sets up handler you'll use to send trackpad commands to OpenVR with, in the Y direction
	VRDriverInput()->CreateScalarComponent(props, "/input/joystick/x", &joystickXHandle, EVRScalarType::VRScalarType_Absolute,
		EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //Why VRScalarType_Absolute? Take a look at the comments on EVRScalarType.
	//VRDriverInput()->CreateScalarComponent(props, "/input/trackpad/x", &trackpadXHandle, EVRScalarType::VRScalarType_Absolute,
	//	EVRScalarUnits::VRScalarUnits_NormalizedTwoSided); //Why VRScalarUnits_NormalizedTwoSided? Take a look at the comments on EVRScalarUnits.
	
	//The following properites are ones I tried out because I saw them in other samples, but I found they were not needed to get the sample working.
	//There are many samples, take a look at the openvr_header.h file. You can try them out.

	//VRProperties()->SetUint64Property(props, Prop_CurrentUniverseId_Uint64, 2);
	//VRProperties()->SetBoolProperty(props, Prop_HasControllerComponent_Bool, true);
	//VRProperties()->SetBoolProperty(props, Prop_NeverTracked_Bool, true);
	//VRProperties()->SetInt32Property(props, Prop_Axis0Type_Int32, k_eControllerAxis_TrackPad);
	//VRProperties()->SetInt32Property(props, Prop_Axis2Type_Int32, k_eControllerAxis_Joystick);
	//VRProperties()->SetStringProperty(props, Prop_SerialNumber_String, "example_controler_serial");
	//VRProperties()->SetStringProperty(props, Prop_RenderModelName_String, "vr_controller_vive_1_5");
	//uint64_t availableButtons = ButtonMaskFromId(k_EButton_SteamVR_Touchpad) |
	//	ButtonMaskFromId(k_EButton_IndexController_JoyStick);
	//VRProperties()->SetUint64Property(props, Prop_SupportedButtons_Uint64, availableButtons);

	return VRInitError_None;
}

DriverPose_t ControllerDriver::GetPose()
{
	DriverPose_t pose = { 0 }; //This example doesn't use Pose, so this method is just returning a default Pose.
	pose.poseIsValid = false;
	pose.result = TrackingResult_Calibrating_OutOfRange;
	pose.deviceIsConnected = true;

	HmdQuaternion_t quat;
	quat.w = 1;
	quat.x = 0;
	quat.y = 0;
	quat.z = 0;

	pose.qWorldFromDriverRotation = quat;
	pose.qDriverFromHeadRotation = quat;

	return pose;
}

vr::TrackedDevicePose_t GetHMDPose() {
	vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
	VRServerDriverHost()->GetRawTrackedDevicePoses(0, poses, vr::k_unMaxTrackedDeviceCount);

	// L'HMD dovrebbe sempre avere l'ID 0
	return poses[vr::k_unTrackedDeviceIndex_Hmd];
}


void ControllerDriver::RunFrame() {
	XINPUT_STATE state;
	DWORD result = XInputGetState(0, &state);

	if (result == ERROR_SUCCESS) {
		// Leggi gli assi X e Y dal controller
		float xAxis = state.Gamepad.sThumbLX / 32768.0f; // Normalizza il valore tra -1 e 1
		float yAxis = state.Gamepad.sThumbLY / 32768.0f; // Normalizza il valore tra -1 e 1

		// Ottieni la posa dell'HMD
		vr::TrackedDevicePose_t hmdPose = GetHMDPose();

		// Verifica se la posa è valida
		if (hmdPose.bPoseIsValid) {
			// Estrai la matrice di trasformazione dal TrackedDevicePose_t
			vr::HmdMatrix34_t mat = hmdPose.mDeviceToAbsoluteTracking;

			// Estrai i vettori forward e right dalla matrice
			float forward[2] = { mat.m[2][0], mat.m[2][2] };
			float right[2] = { mat.m[0][0], mat.m[0][2] };

			// Normalizza i vettori
			float forwardMagnitude = sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
			float rightMagnitude = sqrt(right[0] * right[0] + right[1] * right[1]);
			forward[0] /= forwardMagnitude;
			forward[1] /= forwardMagnitude;
			right[0] /= rightMagnitude;
			right[1] /= rightMagnitude;

			// Ruota l'input del tappeto in base all'orientamento della testa
			// Utilizzando una matrice di rotazione inversa
			float rotatedX = -yAxis * forward[0] + xAxis * right[0];
			float rotatedY = -yAxis * forward[1] + xAxis * right[1];

			// Inverti l'asse delle y
			rotatedY *= -1;

			// Aggiorna i componenti scalari degli assi X e Y con i valori ruotati
			VRDriverInput()->UpdateScalarComponent(joystickXHandle, rotatedX, 0);
			VRDriverInput()->UpdateScalarComponent(joystickYHandle, rotatedY, 0);
		}
		else {
			// Se la posa non è valida, usa i valori originali dell'analogico
			VRDriverInput()->UpdateScalarComponent(joystickXHandle, xAxis, 0);
			VRDriverInput()->UpdateScalarComponent(joystickYHandle, yAxis, 0);
		}
	}
	else {
		// Se non riesce a leggere lo stato del controller, potrebbe non essere connesso
		// Puoi gestire questa situazione come desideri, ad esempio mantenendo i valori precedenti
		// o segnalando un errore
	}
}






void ControllerDriver::Deactivate()
{
	driverId = k_unTrackedDeviceIndexInvalid;
}

void* ControllerDriver::GetComponent(const char* pchComponentNameAndVersion)
{
	//I found that if this method just returns null always, it works fine. But I'm leaving the if statement in since it doesn't hurt.
	//Check out the IVRDriverInput_Version declaration in openvr_driver.h. You can search that file for other _Version declarations 
	//to see other components that are available. You could also put a log in this class and output the value passed into this 
	//method to see what OpenVR is looking for.
	if (strcmp(IVRDriverInput_Version, pchComponentNameAndVersion) == 0)
	{
		return this;
	}
	return NULL;
}

void ControllerDriver::EnterStandby() {}

void ControllerDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) 
{
	if (unResponseBufferSize >= 1)
	{
		pchResponseBuffer[0] = 0;
	}
}