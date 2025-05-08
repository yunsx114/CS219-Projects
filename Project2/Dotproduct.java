import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.StringTokenizer;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.StringTokenizer;

public class JavaVecMulti {

    static final int CYCLE_TIME = 100;

    public static void main(String[] args) throws IOException {
        String inputFile = "randomCase.txt";
        QReader sc = new QReader(new File(inputFile));
        int type = sc.nextInt();
        int dim = sc.nextInt();
        int N = sc.nextInt();

        double sum = 0;
        long totalTime = 0;

        for (int i = 0; i < 10000; i++) System.nanoTime();

        switch (type) {
            case 1 -> { // int
                int[] intData = new int[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        intData[i * dim + j] = sc.nextInt();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorInt(intData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 2 -> { // short
                short[] shortData = new short[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        shortData[i * dim + j] = sc.nextShort();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorShort(shortData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 3 -> { // byte
                byte[] byteData = new byte[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        byteData[i * dim + j] = sc.nextByte();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorByte(byteData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 4 -> { // double
                double[] doubleData = new double[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        doubleData[i * dim + j] = sc.nextDouble();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorDouble(doubleData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            case 5 -> { // float
                float[] floatData = new float[N * dim];
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < dim; j++) {
                        floatData[i * dim + j] = sc.nextFloat();
                    }
                }
                for (int i = 0; i < CYCLE_TIME; i++) {
                    long start = System.nanoTime();
                    sum = calculateVectorFloat(floatData, N, dim);
                    totalTime += System.nanoTime() - start;
                }
            }
            default -> throw new IllegalArgumentException("Invalid type: " + type);
        }

        long avgTime = totalTime / CYCLE_TIME;
        System.out.printf("sum = %f\n", sum);
        System.out.println("Calculations finished in avg "+avgTime+" nano second");
        System.out.println("Calculations finished in avg "+avgTime/1e6+" millisecond");
    }


    public static int calculateVectorInt(int[] flatData, int N, int dim) {
        int sum = 0;
        for (int i = 1; 2 * i <= N; i++) {
            int offset1 = (2 * i - 2) * dim;
            int offset2 = (2 * i - 1) * dim;
            for (int j = 0; j < dim; j++) {
                sum += flatData[offset1 + j] * flatData[offset2 + j];
            }
        }
        return sum;
    }

    public static int calculateVectorShort(short[] data, int N, int dim) {
        int sum = 0;
        for (int i = 1; 2 * i <= N; i++) {
            int p1 = (2 * i - 2) * dim, p2 = (2 * i - 1) * dim;
            for (int j = 0; j < dim; j++) {
                sum += data[p1 + j] * data[p2 + j];
            }
        }
        return sum;
    }

    public static int calculateVectorByte(byte[] data, int N, int dim) {
        int sum = 0;
        for (int i = 1; 2 * i <= N; i++) {
            int p1 = (2 * i - 2) * dim, p2 = (2 * i - 1) * dim;
            for (int j = 0; j < dim; j++) {
                sum += data[p1 + j] * data[p2 + j];
            }
        }
        return sum;
    }

    public static float calculateVectorFloat(float[] data, int N, int dim) {
        float sum = 0;
        for (int i = 1; 2 * i <= N; i++) {
            int p1 = (2 * i - 2) * dim, p2 = (2 * i - 1) * dim;
            for (int j = 0; j < dim; j++) {
                sum += data[p1 + j] * data[p2 + j];
            }
        }
        return sum;
    }

    public static double calculateVectorDouble(double[] flatData, int N, int dim) {
        double sum = 0;
        for (int i = 1; 2 * i <= N; i++) {
            int offset1 = (2 * i - 2) * dim;
            int offset2 = (2 * i - 1) * dim;
            for (int j = 0; j < dim; j++) {
                sum += flatData[offset1 + j] * flatData[offset2 + j];
            }
        }
        return sum;
    }
}


class QReader {
    private BufferedReader reader;
    private StringTokenizer tokenizer;

    public QReader(){
        reader = new BufferedReader(new InputStreamReader(System.in));
        tokenizer = new StringTokenizer("");
    }

    public QReader(File file){
        try {
            if (file.isFile() && file.exists()) {
                InputStreamReader inputReader = new InputStreamReader(Files.newInputStream(file.toPath()), StandardCharsets.UTF_8);
                reader = new BufferedReader(inputReader);
            }
        }catch (IOException e){
            System.err.println("File not found: " + e.getMessage());
        }
        tokenizer = new StringTokenizer("");
    }

    private String innerNextLine() {
        try {
            return reader.readLine();
        } catch (IOException e) {
            return null;
        }
    }

    public boolean hasNext() {
        while (!tokenizer.hasMoreTokens()) {
            String nextLine = innerNextLine();
            if (nextLine == null) {
                return false;
            }
            tokenizer = new StringTokenizer(nextLine);
        }
        return true;
    }

    public String nextLine() {
        tokenizer = new StringTokenizer("");
        return innerNextLine();
    }

    public String next() {
        hasNext();
        return tokenizer.nextToken();
    }

    public int nextInt() {
        return Integer.parseInt(next());
    }

    public short nextShort() {
        return Short.parseShort(next());
    }

    public byte nextByte() {
        return Byte.parseByte(next());
    }


    public float nextFloat() {
        return Float.parseFloat(next());
    }

    public double nextDouble() {
        return Double.parseDouble(next());
    }

    public long nextLong() {
        return Long.parseLong(next());
    }
}

class QWriter implements Closeable {
    private BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(System.out));

    public void print(Object object) {
        try {
            writer.write(object.toString());
        } catch (IOException e) {
            return;
        }
    }

    public void println(Object object) {
        try {
            writer.write(object.toString());
            writer.write("\n");
        } catch (IOException e) {
            return;
        }
    }

    public void println() {
        try {
            writer.write("\n");
        } catch (IOException e) {
            return;
        }
    }

    @Override
    public void close() {
        try {
            writer.close();
        } catch (IOException e) {
            return;
        }
    }
}
