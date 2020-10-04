data "aws_iot_endpoint" "endpoint" {
  endpoint_type = "iot:Data-ATS"
}

resource "aws_iot_certificate" "cert" {
  active = true
}

resource "local_file" "secrets_h" {
  content = templatefile("./secrets.tmpl.h", {
    endpoint    = data.aws_iot_endpoint.endpoint.endpoint_address
    device_cert = aws_iot_certificate.cert.certificate_pem
    device_key  = aws_iot_certificate.cert.private_key
    thing_name  = aws_iot_thing.thing.name
  })
  filename = "../esp32/secrets.h"
}