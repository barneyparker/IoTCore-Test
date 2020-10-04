resource "aws_iot_topic_rule" "rule" {
  name        = "MyRule"
  description = "Example rule"
  enabled     = true
  sql         = "SELECT * FROM 'esp32/pub'"
  sql_version = "2016-03-23"

  lambda {
    function_arn = aws_lambda_function.lambda.arn
  }

  error_action {
    lambda {
      function_arn = aws_lambda_function.lambda.arn
    }
  }
}

data "archive_file" "lambda" {
  type        = "zip"
  output_path = "${path.module}/.terraform/iot-handler.zip"
  source_dir  = "${path.module}/src"
}

resource "aws_lambda_function" "lambda" {
  function_name    = "iot-handler"
  filename         = data.archive_file.lambda.output_path
  source_code_hash = data.archive_file.lambda.output_base64sha256

  runtime = "nodejs12.x"
  handler = "index.handler"

  role = aws_iam_role.lambda.arn
}

resource "aws_iam_role" "lambda" {
  name               = "iot-handler"
  assume_role_policy = <<-EOF
    {
      "Version": "2012-10-17",
      "Statement": [
        {
          "Effect": "Allow",
          "Action": "sts:AssumeRole",
          "Principal": {
            "Service": "lambda.amazonaws.com"
          }
        }
      ]
    }
  EOF
}

#resource "aws_lambda_permission" "lambda_permission" {
#  statement_id  = "APIGatewayInvoke"
#  action        = "lambda:InvokeFunction"
#  function_name = aws_lambda_function.lambda.function_name
#  principal     = "iot.amazonaws.com"
#  source_arn    = 
#}

resource "aws_iam_role_policy" "lambda_policy" {
  name   = "iot_handler_policy"
  role   = aws_iam_role.lambda.name
  policy = data.aws_iam_policy_document.lambda_policy.json
}

data "aws_iam_policy_document" "lambda_policy" {
  statement {
    actions = [
      "logs:CreateLogGroup",
      "logs:CreateLogStream",
      "logs:PutLogEvents"
    ]
    resources = ["arn:aws:logs:*:*:*"]
  }

  statement {
    actions = [
      "cloudwatch:PutMetricData"
    ]
    resources = ["*"]
  }
}

resource "aws_cloudwatch_log_group" "log_group" {
  name              = "/aws/lambda/${aws_lambda_function.lambda.function_name}"
  retention_in_days = 7
}