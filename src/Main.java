import java.io.FileWriter;
import java.io.IOException;
import java.util.Random;

public class Main {
    public static void main(String[] args) {
        String outputFile = "randomCase.txt";
        int type;               // 生成类型：1=int, 2=short, 3=signed char, 4=double, 5=float
        boolean is_simple;      // 值范围：简单值（较小）或复杂值（较大）
        boolean std;            // 对于float/double，是否生成标准范围(0-1)
        int dimension;          // 向量维度
        int N;                  // 样本数量 N = 数量 N/2 = 计算次数

        if(args.length == 0) {
            type = 5;
            is_simple = false;
            std = false;
            dimension = 5000;
            N = 10000;
        } else {
            if(args.length < 5) {
                throw new IllegalArgumentException("wrong number of parameter: " + args.length);
            }
            type = Integer.parseInt(args[0]);
            is_simple = Boolean.parseBoolean(args[1]);
            std = Boolean.parseBoolean(args[2]);
            dimension = Integer.parseInt(args[3]);
            N = Integer.parseInt(args[4]);
        }

        Random rand = new Random();  // 随机数生成器

        float float_ctrl = 1e-30f;

        try (FileWriter writer = new FileWriter(outputFile)) {
            writer.write(type + "\n" + dimension + "\n" + N + "\n");
            for (int i = 0; i < N; i++) { // 生成 N 个样本
                StringBuilder sb = new StringBuilder();
                for (int j = 0; j < dimension; j++) {
                    switch (type) {
                        case 1 -> { // int 类型
                            int value = is_simple ? rand.nextInt(256) - 128 : rand.nextInt(Integer.MAX_VALUE);
                            sb.append(value).append(" ");
                        }
                        case 2 -> { // short 类型
                            short value = (short) (is_simple ? rand.nextInt(256) - 128 : rand.nextInt(Short.MAX_VALUE));
                            sb.append(value).append(" ");
                        }
                        case 3 -> { // signed char 类型 (-128 to 127)
                            byte value = (byte) (rand.nextInt(256) - 128);
                            sb.append(value).append(" ");
                        }
                        case 4 -> { // double 类型
                            double value;
                            if (std) {
                                value = rand.nextDouble(); // [0, 1) 标准范围
                            } else {
                                if (is_simple) {
                                    value = rand.nextDouble() * 256 - 128;
                                } else {
                                    //value = rand.nextDouble() * 2 * Double.MAX_VALUE - Double.MAX_VALUE;
                                    value = (Double.MAX_VALUE * float_ctrl) * rand.nextDouble() * (rand.nextInt(2)*2-1);
                                }
                            }
                            sb.append(value).append(" ");
                        }
                        case 5 -> { // float 类型
                            double value;
                            if (std) {
                                value = rand.nextFloat(); // [0, 1) 标准范围
                            } else {
                                if (is_simple) {
                                    value = rand.nextFloat() * 256 - 128;
                                } else {
                                    value = (Float.MAX_VALUE * float_ctrl) * rand.nextFloat() * (rand.nextInt(2)*2-1);
                                }
                            }
                            sb.append(value).append(" ");
                        }
                        default -> throw new IllegalArgumentException("Invalid type: " + type);
                    }
                }
                sb.append("\n");
                writer.write(sb.toString());
            }

            System.out.println("File generated: " + outputFile);

        } catch (IOException e) {
            System.err.println("Failed generating the file: " + e.getMessage());
        }
    }
}
/*
java -cp -XX:+UnlockDiagnosticVMOptions out JavaVecMulti
*/