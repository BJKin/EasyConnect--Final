import numpy as np
import pandas as pd
import os
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout, Input, GlobalAveragePooling1D, BatchNormalization, AveragePooling1D, UpSampling1D, Add, Activation
from tensorflow.keras.utils import to_categorical
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime

np.random.seed(42)
tf.random.set_seed(42)

def loadCSV(filepath, feature_columns=None):
    try:
        dataframe = pd.read_csv(filepath) 
        
        if feature_columns is not None:
            available_columns = [col for col in feature_columns if col in dataframe.columns]
            if available_columns:
                dataframe = dataframe[available_columns]
        else:
            available_columns = dataframe.columns.tolist()
            dataframe = dataframe[available_columns]

        data = dataframe.values
        # print(f"Data shape: {data.shape}")
        return data
        
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None

def create_overlapping_windows(data, window_size=125, num_windows=3):
    total_length = data.shape[0]
    step_size = (total_length - window_size) // (num_windows - 1)
    
    windows = []
    for i in range(num_windows):
        start_idx = i * step_size
        end_idx = start_idx + window_size
    
        if end_idx > total_length:
            start_idx = total_length - window_size
            end_idx = total_length
        
        window = data[start_idx:end_idx]
        windows.append(window)
    
    return windows

def addNoise(data, noise_factor=0.01):
    noise = np.random.normal(0, noise_factor, data.shape)
    noisy_data = data + noise
    return noisy_data

def randomScaling(data, scale_range=(0.8, 1.2)):
    scale_factor = np.random.uniform(scale_range[0], scale_range[1])
    scaled_data = data * scale_factor
    return scaled_data

def randomRotation(data, angle_range=15.0):
    angles = np.random.uniform(-angle_range, angle_range, 3) * np.pi / 180
    
    Rx = np.array([[1, 0, 0], [0, np.cos(angles[0]), -np.sin(angles[0])], [0, np.sin(angles[0]), np.cos(angles[0])]])
    Ry = np.array([[np.cos(angles[1]), 0, np.sin(angles[1])], [0, 1, 0], [-np.sin(angles[1]), 0, np.cos(angles[1])]])
    Rz = np.array([[np.cos(angles[2]), -np.sin(angles[2]), 0], [np.sin(angles[2]), np.cos(angles[2]), 0], [0, 0, 1]])
    
    R = Rz @ Ry @ Rx
    
    rotated_data = data.copy()
    accel_data = data[:, :3] 
    rotated_accel = (R @ accel_data.T).T 
    rotated_data[:, :3] = rotated_accel
    
    gyro_data = data[:, 3:6] 
    rotated_gyro = (R @ gyro_data.T).T 
    rotated_data[:, 3:6] = rotated_gyro
    
    return rotated_data

def padOrTruncate(data, target_length):
    if len(data) == target_length:
        return data
    elif len(data) < target_length:
        padding = np.zeros((target_length - len(data), data.shape[1]))
        return np.vstack([data, padding])
    else:
        return data[:target_length]
    
def loadData(base_directory, feature_columns=None, include_classes=None):
    class_names = []
    
    all_samples = []
    all_labels = []
    
    available_gestures = []
    for item in sorted(os.listdir(base_directory)):
        item_path = os.path.join(base_directory, item)
        if os.path.isdir(item_path):
            available_gestures.append(item)
    
    if include_classes is not None:
        gestures_to_process = [g for g in available_gestures if g in include_classes]
    else:
        gestures_to_process = available_gestures

    for gesture_name in gestures_to_process:
        gesture_path = os.path.join(base_directory, gesture_name)
        
        class_names.append(gesture_name)
        class_label = len(class_names) - 1 
        gesture_samples = [] 
        try:
            csv_files = [f for f in os.listdir(gesture_path) if f.endswith('.csv')]
        except OSError as e:
            print(f"Error accessing {gesture_path}: {e}")
            continue

        for csv_file in csv_files:
            csv_path = os.path.join(gesture_path, csv_file)
            if feature_columns is not None:
                sensor_data = loadCSV(csv_path, feature_columns)
            else:
                sensor_data = loadCSV(csv_path)
            
            windows = create_overlapping_windows(sensor_data, 125, 3)
            for window in windows:
                gesture_samples.append(window)
                gesture_samples.append(padOrTruncate(addNoise(window), 125))
                gesture_samples.append(padOrTruncate(randomScaling(window), 125))
                gesture_samples.append(padOrTruncate(randomRotation(window), 125))
        
        if gesture_samples:
            all_samples.extend(gesture_samples)
            all_labels.extend([class_label] * len(gesture_samples))
            print(f"  Total samples for {gesture_name}: {len(gesture_samples)}")
    
    if all_samples:
        X = np.array(all_samples)
        y = np.array(all_labels)
        
        print(f"\nFINAL DATASET:")
        print(f"Shape: {X.shape}")
        print(f"Total samples: {len(X)}")
        print(f"Classes: {class_names}")
        print(f"Samples per class: {np.bincount(y)}")
        
        return X, y, class_names

def buildDenseModel(timesteps, features, classes):
    model = Sequential([
        Input(shape=(timesteps, features)),
        Dense(50, activation='relu'),
        Dense(15, activation='relu'),
        GlobalAveragePooling1D(),
        Dense(classes, activation='softmax')
    ])
    model.compile(loss='mse', optimizer='rmsprop', metrics=['accuracy'])

    return model

def buildDenseModel2(timesteps, features, classes):
    model = Sequential([
        Input(shape=(timesteps, features)),
        Dense(64, activation='relu'),
        Dense(32, activation='relu'),
        Dense(16, activation='relu'),
        GlobalAveragePooling1D(),
        Dense(classes, activation='softmax') 
    ])
    model.compile(loss='mse', optimizer='rmsprop', metrics=['accuracy'])

    return model

def buildDenseModelNormalized(timesteps, features, classes):
    model = Sequential([
    Input(shape=(timesteps, features)),
    Dense(64, activation='relu'),
    BatchNormalization(),
    Dropout(0.3),
    Dense(32, activation='relu'),
    BatchNormalization(), 
    Dropout(0.3),
    Dense(16, activation='relu'),
    Dropout(0.2),
    GlobalAveragePooling1D(),
    Dense(classes, activation='softmax')
    ])
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

    return model

def buildDenseModelMulti(timesteps, features, classes):
    input_layer = Input(shape=(timesteps, features))

    branch1 = Dense(64, activation='relu')(input_layer)
    branch1 = Dense(32, activation='relu')(branch1)
    branch1 = Dense(16, activation='relu')(branch1)

    branch2 = AveragePooling1D(pool_size=1)(input_layer)
    branch2 = Dense(64, activation='relu')(branch2)
    branch2 = Dense(32, activation='relu')(branch2)
    branch2 = Dense(16, activation='relu')(branch2)
    branch2 = UpSampling1D(size=1)(branch2)

    combined = tf.keras.layers.Concatenate()([branch1, branch2])
    combined = Dense(24, activation='relu')(combined)
    pooled = GlobalAveragePooling1D()(combined)
    output = Dense(classes, activation='softmax')(pooled)

    model = tf.keras.Model(inputs=input_layer, outputs=output)
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

    return model

def buildDenseModelWide(timesteps, features, classes):
    model = Sequential([
    Input(shape=(timesteps, features)),
    Dense(128, activation='relu'),
    BatchNormalization(),
    Dropout(0.3),
    Dense(64, activation='relu'),
    BatchNormalization(),
    Dropout(0.2),
    Dense(32, activation='relu'),
    GlobalAveragePooling1D(),
    Dense(classes, activation='softmax')
    ])
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

    return model

def residual_block(input_tensor, units, dropout_rate=0.3):
    x = Dense(units, activation='relu')(input_tensor)
    x = BatchNormalization()(x)
    x = Dropout(dropout_rate)(x)
    x = Dense(units, activation='relu')(x)
    x = BatchNormalization()(x)
    
    if input_tensor.shape[-1] == units:
        shortcut = input_tensor
    else:
        shortcut = Dense(units, activation='linear')(input_tensor)
    
    output = Add()([x, shortcut])
    output = Activation('relu')(output)
    
    return output

def buildResNetModel(timesteps, features, classes):
    input_layer = Input(shape=(timesteps, features))

    x = Dense(64, activation='relu')(input_layer)
    x = BatchNormalization()(x)

    x = residual_block(x, 64, dropout_rate=0.3)
    x = residual_block(x, 32, dropout_rate=0.3)  
    x = residual_block(x, 16, dropout_rate=0.2) 

    x = GlobalAveragePooling1D()(x)
    x = Dropout(0.5)(x)
    output = Dense(classes, activation='softmax')(x)
    
    model = tf.keras.Model(inputs=input_layer, outputs=output)
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
    
    return model

def trainModel(X, y, class_names, epochs, batch_size):
    y_categorical = to_categorical(y)
    n_classes = len(class_names)
    
    X_train, X_test, y_train, y_test = train_test_split(X, y_categorical, random_state=42, stratify=y)
    print(f"Training samples: {len(X_train)}")
    print(f"Testing samples: {len(X_test)}")
    
    n_timesteps, n_features = X.shape[1], X.shape[2]
    model = buildDenseModelMulti(n_timesteps, n_features, n_classes)
    model.summary()
    
    history = model.fit(X_train, y_train, epochs=epochs, batch_size=batch_size, validation_split=0.2, verbose=1, shuffle=True)
    return model, history, X_test, y_test, X_train
    
def testModel(model, history, X_test, y_test, class_names, timestamp):
    test_loss, test_accuracy = model.evaluate(X_test, y_test, verbose=0)
    print(f"Test Accuracy: {test_accuracy:.4f}")
    print(f"Test Loss: {test_loss:.4f}")
    
    y_pred = model.predict(X_test)
    y_pred_classes = np.argmax(y_pred, axis=1)
    y_test_classes = np.argmax(y_test, axis=1)
    
    cm = confusion_matrix(y_test_classes, y_pred_classes)
    plt.figure(figsize=(8, 6))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=class_names, yticklabels=class_names)
    plt.title('Confusion Matrix')
    plt.xlabel('Predicted')
    plt.ylabel('Actual')
    plt.tight_layout()
    plt.savefig(f"./TensorFlow/ModelAnalysis/confusion_matrix_{timestamp}.png", dpi=300)
    plt.show()
    
    plt.figure(figsize=(12, 4))
    plt.plot(history.history['accuracy'], label='Training Accuracy')
    plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
    plt.title('Model Accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"./TensorFlow/ModelAnalysis/train_val_accuracy_{timestamp}.png", dpi=300)
    plt.show()

def exportModel(model, save_path):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    with open(save_path, 'wb') as f:
        f.write(tflite_model)

    return tflite_model

if __name__ == "__main__":    
    try:
        features = ["lin_acc_x", "lin_acc_y", "lin_acc_z", "gyro_x", "gyro_y", "gyro_z"]
        classes = ["dapup", "fistbump", "flapping", "handshake", "highfive", "still", "walking"]

        X, y, class_names = loadData("./TensorFlow/Data", feature_columns=features)
        model, history, X_test, y_test, X_train = trainModel(X, y, class_names, 1000, 64)
        timestamp = datetime.now().strftime("%m%d_%H%M")
        testModel(model, history, X_test, y_test, class_names, timestamp)
        tflite_model = exportModel(model, f"./TensorFlow/Models/Handshake_{timestamp}.tflite")
        
        # To convert to header file compatible with Arduino, use the following command:
        # xxd -i Handshake_0604_1400.tflite > Handshake_0604_1400.h
    except Exception as e:
        print(f"Error during training: {e}")