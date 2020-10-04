const CW = require('aws-sdk/clients/cloudwatch')

const cw = new CW({apiVersion: '2010-08-01'})

module.exports.handler = async (event) => {

  console.log(JSON.stringify(event, null, 2))

  // Add the uploaded metrics to CloudWatch

  const params = {
    MetricData: [],
    Namespace: 'esp_iot'
  }

  params.MetricData.push({
    MetricName: 'LightLevel',
    Dimensions: [
      {
        Name: 'DeviceID',
        Value: event.device
      }
    ],
    Unit: 'None',
    Value: event.light,
    Timestamp: new Date(event.epoch * 1000)
  })

  params.MetricData.push({
    MetricName: 'Uptime',
    Dimensions: [
      {
        Name: 'DeviceID',
        Value: event.device
      }
    ],
    Unit: 'Seconds',
    Value: event.uptime,
    Timestamp: new Date(event.epoch * 1000)
  })

  console.log(params)

  try {
    const result = await cw.putMetricData(params).promise()

    console.log(result)
    
    // Return OK
    return true
  } catch(err) {
    console.log(err)
    return false
  }

}
