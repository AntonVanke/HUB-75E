export const sensorInputIds=['data-temperature','data-humidity','data-message','data-temperature-valid','data-humidity-valid','data-imu-supported','data-imu-valid','data-accel-x','data-accel-y','data-accel-z','data-gyro-x','data-gyro-y','data-gyro-z','data-screen-rotation'];
const finite=value=>value!==''&&value!==null&&value!==undefined&&Number.isFinite(Number(value));
export function accelerationForGravity(x,y,z,rotation){
  const [bx,by]=rotation===90?[-y,x]:rotation===180?[-x,-y]:rotation===270?[y,-x]:[x,y];
  return {accel_x:-bx,accel_y:-by,accel_z:-z};
}
export function sensorPreviewData(input){
  const rotation=[0,90,180,270].includes(Number(input.screen_rotation))?Number(input.screen_rotation):270;
  const output={message:input.message??'',screen_rotation:rotation,auto_rotate:false,sensor_source:'simulated'};
  for(const key of ['temperature','humidity']){
    output[key+'_valid']=input[key+'_valid']===true&&finite(input[key]);
    if(output[key+'_valid'])output[key]=Number(input[key]);
  }
  output.imu_supported=input.imu_supported===true;
  const axes=['accel_x','accel_y','accel_z','gyro_x','gyro_y','gyro_z'];
  output.imu_valid=output.imu_supported&&input.imu_valid===true&&axes.every(key=>finite(input[key]));
  if(output.imu_valid){
    for(const key of axes)output[key]=Number(input[key]);
    const bx=-output.accel_x,by=-output.accel_y;
    [output.gravity_x,output.gravity_y]=rotation===90?[by,-bx]:rotation===180?[-bx,-by]:rotation===270?[-by,bx]:[bx,by];
    output.gravity_z=-output.accel_z;
  }
  return output;
}
